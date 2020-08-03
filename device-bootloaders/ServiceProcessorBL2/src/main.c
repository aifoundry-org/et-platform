#include "serial.h"
#include "interrupt.h"
#include "dummy_isr.h"
#include "mailbox.h"
#include "pcie.h"

#include "FreeRTOS.h"
#include "task.h"

#include "io.h"
#include "service_processor_ROM_data.h"
#include "service_processor_BL1_data.h"
#include "service_processor_BL2_data.h"
#include "bl2_certificates.h"
#include "bl2_firmware_loader.h"
#include "bl2_flash_fs.h"
#include "bl2_build_configuration.h"

#include "bl2_main.h"
#include "bl2_timer.h"
#include "bl2_flashfs_driver.h"
#include "bl2_vaultip_driver.h"
#include "bl2_reset.h"
#include "bl2_sp_pll.h"
#include "sp_otp.h"
#include "bl2_sp_memshire_pll.h"
#include "bl2_minion_pll_and_dll.h"
#include "bl2_ddr_init.h"

#include "etsoc_hal/inc/rm_esr.h"
#include "etsoc_hal/inc/hal_device.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "bl2_crypto.h"

#define TASK_STACK_SIZE 4096 // overkill for now

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize);
void vApplicationIdleHook(void);
void vApplicationTickHook(void);
void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName);

static SERVICE_PROCESSOR_BL2_DATA_t g_service_processor_bl2_data;

SERVICE_PROCESSOR_BL2_DATA_t * get_service_processor_bl2_data(void) {
    return &g_service_processor_bl2_data;
}

bool is_vaultip_disabled(void) {
    uint32_t rm_status2;
    static bool initialized = false;
    static bool vaultip_disabled = false;

    if (!initialized) {
        if (0 != sp_otp_get_vaultip_chicken_bit(&vaultip_disabled)) {
            vaultip_disabled = false;
        }
        rm_status2 = ioread32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_STATUS2_ADDRESS);
        if (0 != RESET_MANAGER_RM_STATUS2_A0_UNLOCK_GET(rm_status2) && 0 != RESET_MANAGER_RM_STATUS2_SKIP_VAULT_GET(rm_status2)) {
            vaultip_disabled = true;
        }
    }

    return vaultip_disabled;
}

static uint64_t calculate_minion_shire_enable_mask(void) {
    int ret;
    OTP_NEIGHBORHOOD_STATUS_NH128_NH135_OTHER_t status_other;
    uint64_t enable_mask = 0;

    // 32 Worker Shires: There are 4 OTP entries containing the status of their Neighboorhods
    for (uint32_t entry = 0; entry < 4; entry++) {
        uint32_t status;

        ret = sp_otp_get_neighborhood_status_mask(entry, &status);
        if (ret < 0) {
            // If the OTP read fails, assume we have to enable all Neighboorhods
            status = 0xFFFFFFFF;
        }

        // Each Neighboorhod status OTP entry contains information for 8 Shires
        for (uint32_t i = 0; i < 8; i++) {
            // Only enable a Shire if *ALL* its Neighboorhods are Functional
            if ((status & 0xF) == 0xF) {
                enable_mask |= 1ULL << (entry * 8 + i);
            }
            status >>= 4;
        }
    }

    // Master Shire Neighboorhods status
    ret = sp_otp_get_neighborhood_status_nh128_nh135_other(&status_other);
    if ((ret < 0) || ((status_other.B.neighborhood_status & 0xF) == 0xF)) {
        enable_mask |= 1ULL << 32;
    }

    return enable_mask;
}

static TaskHandle_t gs_taskHandleMain;
static StackType_t gs_stackMain[TASK_STACK_SIZE];
static StaticTask_t gs_taskBufferMain;

static void taskMain(void *pvParameters)
{
    uint64_t minion_shires_mask;
    (void)pvParameters;

    // Disable buffering on stdout
    setbuf(stdout, NULL);

    // In non-fast-boot mode, the bootrom initializes PCIe link
#if FAST_BOOT
    PCIe_init(false /*expect_link_up*/);
#else
    PCIe_init(true /*expect_link_up*/);
#endif

    MBOX_init_pcie();
    printf("Mailbox to host initialized.\n");

    minion_shires_mask = calculate_minion_shire_enable_mask();

    printf("---------------------------------------------\n");
    printf("Minion shires to enable: 0x%" PRIx64 "\n", minion_shires_mask);
    printf("Starting Minions reset release sequence...\n");

    if (0 != configure_sp_pll_2()) {
        printf("configure_sp_pll_2() failed!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    if (0 != configure_sp_pll_4()) {
        printf("configure_sp_pll_4() failed!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    printf("SP PLLs 2 & 4 configured and locked.\n");

    if (0 != release_memshire_from_reset()) {
        printf("release_memshire_from_reset() failed!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    if (0 != configure_memshire_plls()) {
        printf("configure_memshire_plls() failed!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    printf("SP MemShire PLLs configured and locked.\n");

#if !FAST_BOOT
    if (0 != ddr_config()) {
        printf("ddr_config() failed!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    printf("DRAM ready.\n");
#endif

    if (0 != release_minions_from_cold_reset()) {
        printf("release_minions_from_cold_reset() failed!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    printf("Released Minion shires from cold reset.\n");

    if (0 != release_minions_from_warm_reset()) {
        printf("release_minions_from_warm_reset() failed!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    printf("Released Minions from warm reset.\n");

    if (0 != configure_minion_plls_and_dlls(minion_shires_mask)) {
        printf("configure_minion_plls_and_dlls() failed!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    printf("Minion shires PLLs and DLLs configured.\n");

    printf("---------------------------------------------\n");
    printf("Attempting to load SW ROOT/Issuing Certificate chain...\n");
    if (0 != load_sw_certificates_chain()) {
        printf("Failed to load SW ROOT/Issuing Certificate chain!\n");
        goto FIRMWARE_LOAD_ERROR;
    }

    // In fast-boot mode we skip loading from flash, and assume everything is already pre-loaded
#if !FAST_BOOT
    printf("---------------------------------------------\n");
    printf("Attempting to load Machine Minion firmware...\n");
    if (0 != load_firmware(ESPERANTO_IMAGE_TYPE_MACHINE_MINION)) {
        printf("Failed to load Machine Minion firmware!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    printf("Machine Minion firmware loaded.\n");

    printf("---------------------------------------------\n");
    printf("Attempting to load Master Minion firmware...\n");
    if (0 != load_firmware(ESPERANTO_IMAGE_TYPE_MASTER_MINION)) {
        printf("Failed to load Master Minion firmware!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    printf("Master Minion firmware loaded.\n");

    printf("---------------------------------------------\n");
    printf("Attempting to load Worker Minion firmware...\n");
    if (0 != load_firmware(ESPERANTO_IMAGE_TYPE_WORKER_MINION)) {
        printf("Failed to load Worker Minion firmware!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    printf("Worker Minion firmware loaded.\n");
#endif

    printf("---------------------------------------------\n");
    printf("time: %lu\n", timer_get_ticks_count());

    if (0 != enable_minion_neighborhoods(minion_shires_mask)) {
        printf("Failed to enable minion neighborhoods!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    printf("Minion neighborhoods enabled.\n");

    if (0 != enable_minion_threads(minion_shires_mask)) {
        printf("Failed to enable minion threads!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    printf("Minion threads enabled.\n");

    MBOX_init_pcie();
    printf("Mailbox to MM initialized.\n");

    goto DONE;

FIRMWARE_LOAD_ERROR:
    printf("Fatal error... waiting for reset!\n");

DONE:
    while (1)
    {
        printf("M");
        vTaskDelay(2U);
    }
}

static int copy_bl1_data(const SERVICE_PROCESSOR_BL1_DATA_t * bl1_data) {
    printf("SP BL1 data address: %p\n", bl1_data);
    if (NULL == bl1_data || sizeof(SERVICE_PROCESSOR_BL1_DATA_t) != bl1_data->service_processor_bl1_data_size || SERVICE_PROCESSOR_BL1_DATA_VERSION != bl1_data->service_processor_bl1_version) {
        printf("Invalid BL1 DATA!\n");
        return -1;
    }

    g_service_processor_bl2_data.service_processor_rom_version = bl1_data->service_processor_rom_version;
    g_service_processor_bl2_data.service_processor_bl1_version = bl1_data->service_processor_bl1_version;
    g_service_processor_bl2_data.sp_gpio_pins = bl1_data->sp_gpio_pins;
    g_service_processor_bl2_data.vaultip_coid_set = bl1_data->vaultip_coid_set;
    g_service_processor_bl2_data.spi_controller_rx_baudrate_divider = bl1_data->spi_controller_rx_baudrate_divider;
    g_service_processor_bl2_data.spi_controller_tx_baudrate_divider = bl1_data->spi_controller_tx_baudrate_divider;

    // copy the SP ROOT/ISSUING CA certificates chain
    memcpy(&(g_service_processor_bl2_data.sp_certificates), &(bl1_data->sp_certificates), sizeof(bl1_data->sp_certificates));

    // copy the SP BL1 header
    memcpy(&(g_service_processor_bl2_data.sp_bl1_header), &(bl1_data->sp_bl1_header), sizeof(bl1_data->sp_bl1_header));

    // copy the SP BL2 header
    memcpy(&(g_service_processor_bl2_data.sp_bl2_header), &(bl1_data->sp_bl2_header), sizeof(bl1_data->sp_bl2_header));

    return 0;
}

void bl2_main(const SERVICE_PROCESSOR_BL1_DATA_t * bl1_data);

void bl2_main(const SERVICE_PROCESSOR_BL1_DATA_t * bl1_data)
{
    bool vaultip_disabled;
    const IMAGE_VERSION_INFO_t * image_version_info = get_image_version_info();

    // Disable buffering on stdout
    setbuf(stdout, NULL);

    // In non-fast-boot mode, the bootrom initializes SPIO UART0
#if FAST_BOOT
    SERIAL_init(UART0);
#endif

    printf("\n*** SP BL2 STARTED ***\r\n");
    printf("BL2 version: %u.%u.%u (" BL2_VARIANT ")\n", image_version_info->file_version_major, image_version_info->file_version_minor, image_version_info->file_version_revision);
    printf("GIT version: " GIT_VERSION_STRING "\n");
    // printf("GIT hash: " GIT_HASH_STRING "\n");

    memset(&g_service_processor_bl2_data, 0, sizeof(g_service_processor_bl2_data));
    g_service_processor_bl2_data.service_processor_bl2_data_size = sizeof(g_service_processor_bl2_data);
    g_service_processor_bl2_data.service_processor_bl2_version = SERVICE_PROCESSOR_BL2_DATA_VERSION;

    if (0 != copy_bl1_data(bl1_data)) {
        printf("copy_bl1_data() failed!!\n");
        goto FATAL_ERROR;
    }

    timer_init(bl1_data->timer_raw_ticks_before_pll_turned_on, bl1_data->sp_pll0_frequency);

    if (0 != sp_otp_init()) {
        printf("sp_otp_init() failed!\n");
        goto FATAL_ERROR;
    }

    vaultip_disabled = is_vaultip_disabled();

    SERIAL_init(UART1);
    SERIAL_init(PU_UART0);
    SERIAL_init(PU_UART1);

    INT_init();

    if (0 != pll_init(bl1_data->sp_pll0_frequency, bl1_data->sp_pll1_frequency, bl1_data->pcie_pll0_frequency)) {
        printf("pll_init() failed!\n");
        goto FATAL_ERROR;
    }

    if (vaultip_disabled) {
        printf("VaultIP is disabled!\n");
    } else {
        if (0 != vaultip_drv_init()) {
            printf("vaultip_drv_init() failed!\n");
            goto FATAL_ERROR;
        }
        if (0 != crypto_init(bl1_data->vaultip_coid_set)) {
            printf("crypto_init() failed!\n");
            goto FATAL_ERROR;
        }
    }

    // In fast-boot mode we skip loading from flash, and assume everything is already pre-loaded
#if !FAST_BOOT
    if (0 != flashfs_drv_init(&g_service_processor_bl2_data.flash_fs_bl2_info, &bl1_data->flash_fs_bl1_info)) {
        printf("flashfs_drv_init() failed!\n");
        goto FATAL_ERROR;
    }
#endif

    printf("Starting RTOS...\n");

    gs_taskHandleMain = xTaskCreateStatic(taskMain,
                                    "Main Task",
                                    TASK_STACK_SIZE,
                                    NULL,
                                    1,
                                    gs_stackMain,
                                    &gs_taskBufferMain);
    if (gs_taskHandleMain == NULL) {
        printf("xTaskCreateStatic(taskMain) failed!\r\n");
    }

    vTaskStartScheduler();

FATAL_ERROR:
    printf("Encountered a FATAL ERROR!\n");
    printf("Waiting for RESET!!!\n");
    for(;;);
}

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
    /* If the buffers to be provided to the Idle task are declared inside this
    function then they must be declared static - otherwise they will be allocated on
    the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationIdleHook(void)
{
    // "WFI is available in all of the supported S and M privilege modes,
    //  and optionally available to U-mode for implementations that support U-mode interrupts."
    asm("wfi");
}

void vApplicationTickHook(void)
{
    // TODO FIXME watchdog checking goes here
    //SERIAL_write(UART0, ".", 1);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
}
