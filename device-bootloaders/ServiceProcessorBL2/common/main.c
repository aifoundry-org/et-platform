#include "serial.h"
#include "interrupt.h"
#include "dummy_isr.h"
#include "minion_fw_boot_config.h"
#include "pcie_init.h"

#include "FreeRTOS.h"
#include "task.h"

#include "cache_flush_ops.h"
#include "io.h"
#include "service_processor_ROM_data.h"
#include "service_processor_BL1_data.h"
#include "service_processor_BL2_data.h"
#include "bl2_certificates.h"
#include "bl2_firmware_loader.h"
#include "bl2_firmware_update.h"
#include "bl2_flash_fs.h"
#include "bl2_pmic_controller.h"
#include "bl2_build_configuration.h"

#include "bl2_main.h"
#include "bl2_timer.h"
#include "bl2_flashfs_driver.h"
#include "bl2_vaultip_driver.h"
#include "bl2_reset.h"
#include "bl2_sp_pll.h"
#include "minion_state.h"
#include "sp_otp.h"
#include "bl2_sp_memshire_pll.h"
#include "bl2_minion_pll_and_dll.h"
#include "bl2_ddr_init.h"

#include "etsoc_hal/inc/rm_esr.h"
#include "etsoc_hal/inc/hal_device.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "dm_task.h"
#include "watchdog_task.h"
#include "dm_event_control.h"
#include "bl2_crypto.h"
#include "bl2_asset_trk.h"

#include "command_dispatcher.h"

#define TASK_STACK_SIZE 4096 // overkill for now

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize);
void vApplicationIdleHook(void);
void vApplicationTickHook(void);
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);

static SERVICE_PROCESSOR_BL2_DATA_t g_service_processor_bl2_data;

SERVICE_PROCESSOR_BL2_DATA_t *get_service_processor_bl2_data(void)
{
    return &g_service_processor_bl2_data;
}

bool is_vaultip_disabled(void)
{
    uint32_t rm_status2;
    static bool initialized = false;
    static bool vaultip_disabled = false;

    if (!initialized) {
        if (0 != sp_otp_get_vaultip_chicken_bit(&vaultip_disabled)) {
            vaultip_disabled = false;
        }
        rm_status2 = ioread32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_STATUS2_ADDRESS);
        if (0 != RESET_MANAGER_RM_STATUS2_A0_UNLOCK_GET(rm_status2) &&
            0 != RESET_MANAGER_RM_STATUS2_SKIP_VAULT_GET(rm_status2)) {
            vaultip_disabled = true;
        }
    }

    return vaultip_disabled;
}

static int32_t configure_noc(void)
{
    if (0 != configure_sp_pll_2()) {
        printf("configure_sp_pll_2() failed!\n");
        return NOC_MAIN_CLOCK_CONFIGURE_ERROR;
    }

   // TBD: Configure Main NOC Registers via Regbus
   // Remap NOC ID based on MM OTP value
   // Other potential NOC workarounds

   return SUCCESS;
}

static int32_t configure_memshire(void)
{
    if (0 != release_memshire_from_reset()) {
        printf("release_memshire_from_reset() failed!\n");
        return MEMSHIRE_COLD_RESET_CONFIG_ERROR;
    }
    if (0 != configure_memshire_plls()) {
        printf("configure_memshire_plls() failed!\n");
        return MEMSHIRE_PLL_CONFIG_ERROR;
    }
#if !FAST_BOOT
    if (0 != ddr_config()) {
        printf("ddr_config() failed!\n");
        return MEMSHIRE_DDR_CONFIG_ERROR;
    }
    printf("DRAM ready.\n");
#endif
   return SUCCESS;
}

static int32_t configure_minion(uint64_t minion_shires_mask)
{

    if (0 != configure_sp_pll_4()) {
        printf("configure_sp_pll_4() failed!\n");
        return MINION_STEP_CLOCK_CONFIGURE_ERROR;
    }

    if (0 != release_minions_from_cold_reset()) {
        printf("release_minions_from_cold_reset() failed!\n");
        return MINION_COLD_RESET_CONFIG_ERROR;
    }

    if (0 != release_minions_from_warm_reset()) {
        printf("release_minions_from_warm_reset() failed!\n");
        return MINION_WARM_RESET_CONFIG_ERROR ;
    }

    if (0 != configure_minion_plls_and_dlls(minion_shires_mask)) {
        printf("configure_minion_plls_and_dlls() failed!\n");
        return MINION_PLL_DLL_CONFIG_ERROR;
    }

   return SUCCESS;
}

static int32_t load_autheticate_minion_firmware(void)
{
    if (0 != load_sw_certificates_chain()) {
        printf("Failed to load SW ROOT/Issuing Certificate chain!\n");
        return FW_SW_CERTS_LOAD_ERROR;
    }

    // In fast-boot mode we skip loading from flash, and assume everything is already pre-loaded
#if !FAST_BOOT
    if (0 != load_firmware(ESPERANTO_IMAGE_TYPE_MACHINE_MINION)) {
        printf("Failed to load Machine Minion firmware!\n");
        return FW_MACH_LOAD_ERROR;
    }
    printf("MACH FW loaded.\n");

    if (0 != load_firmware(ESPERANTO_IMAGE_TYPE_MASTER_MINION)) {
        printf("Failed to load Master Minion firmware!\n");
        return FW_MM_LOAD_ERROR;
    }
    printf("MM FW loaded.\n");

    // TODO: Update the following to Log macro - set to INFO/DEBUG
    //printf("Attempting to load Worker Minion firmware...\n");
    if (0 != load_firmware(ESPERANTO_IMAGE_TYPE_WORKER_MINION)) {
        printf("Failed to load Worker Minion firmware!\n");
        return FW_CM_LOAD_ERROR;
    }
    printf("WM FW loaded.\n");
#endif

   return SUCCESS;
}


static uint64_t calculate_minion_shire_enable_mask(void)
{
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

static inline void write_minion_fw_boot_config(uint64_t minion_shires)
{
    volatile minion_fw_boot_config_t *boot_config;
    // Use "High" DDR aliased addresses (bit 38) which are uncacheable by the SP
    boot_config = (volatile minion_fw_boot_config_t *)(FW_MINION_FW_BOOT_CONFIG | (1ULL << 38));
    boot_config->minion_shires = minion_shires;
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

    // Establish connection to PMIC
    setup_pmic();

    // In non-fast-boot mode, the bootrom initializes PCIe link
#if FAST_BOOT
    PCIe_release_pshire_from_reset();
    configure_pcie_pll();
    PCIe_init(false /*expect_link_up*/);
#else
    PCIe_init(true /*expect_link_up*/);
#endif

    minion_shires_mask = calculate_minion_shire_enable_mask();

    Minion_State_Init(minion_shires_mask);

    // Create and Initialize all VQ for SP interfaces (Host and MM)
    sp_intf_init();

    /* Initialize the DIRs */
    DIR_Init();

    /* Set the minion shires available */
    DIR_Set_Minion_Shires(minion_shires_mask);

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_VQ_READY);

    printf("time: %lu\n", timer_get_ticks_count());

   // Setup NOC

    if (0 != configure_noc()) {
        printf("configure_noc() failed!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_NOC_INITIALIZED);

   // Setup MemShire/DDR

    if (0 != configure_memshire()) {
        printf("configure_memshire() failed!\n");
        goto FIRMWARE_LOAD_ERROR;
    }

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_DDR_INITIALIZED);

   // Setup Minions

    if (0 != configure_minion(minion_shires_mask)) {
        printf("Configure Minion failed!\n");
        goto FIRMWARE_LOAD_ERROR;
    }

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_MINION_INITIALIZED);

    // Minion FW Authenticate and load to DDR
    if (0 != load_autheticate_minion_firmware()) {
        printf("Failed to load Minion Firmware!\n");
        goto FIRMWARE_LOAD_ERROR;
    }

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_MINION_FW_AUTHENTICATED_INITIALIZED);

    // Launch Dispatcher

    launch_command_dispatcher();

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_COMMAND_DISPATCHER_INITIALIZED);

    if (0 != enable_minion_neighborhoods(minion_shires_mask)) {
        printf("Failed to enable minion neighborhoods!\n");
        goto FIRMWARE_LOAD_ERROR;
    }

    // Write Minion FW boot config before booting Minion threads up
    write_minion_fw_boot_config(minion_shires_mask);

    // TODO: SW-3877 need to READ Master Shire ID from OTP, potentially reprogram NOC
    if (0 != enable_master_shire_threads(32)) {
        printf("Failed to enable Master minion threads!\n");
        goto FIRMWARE_LOAD_ERROR;
    }

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_MM_FW_LAUNCHED);

    // Program ATUs here
    pcie_enable_link();

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_ATU_PROGRAMMED);

    // init thermal power management service
    if (0 != init_thermal_pwr_mgmt_service()) {
        printf("Failed to init thermal power management!\n");
        goto FIRMWARE_LOAD_ERROR;
    }

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_PM_READY);

    // init watchdog service
     if (0 != init_watchdog_service()) {
        printf("Failed to init watchdog service!\n");
        goto FIRMWARE_LOAD_ERROR;
    }

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_SP_WATCHDOG_TASK_READY);

    // init DM event handler task
    if (0 != dm_event_control_init()) {
        printf("Failed to create dm event handler task!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_EVENT_HANDLER_READY);

#if !FAST_BOOT
    // SP and minions have booted successfully. Increment the completed boot counter
    if (0 != flashfs_drv_increment_completed_boot_count()) {
        printf("Failed to increment the completed boot counter!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    printf("Incremented the completed boot counter!\n");
#endif

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_DEV_READY);
    printf("SP Device Ready!\n");



    // Init DM sampling task
    init_dm_sampling_task();

    goto DONE;

FIRMWARE_LOAD_ERROR:
    printf("Fatal error... waiting for reset!\n");

DONE:
    while (1) {
        printf("M");
        vTaskDelay(2U);
    }
}

static int copy_bl1_data(const SERVICE_PROCESSOR_BL1_DATA_t *bl1_data)
{
    if (NULL == bl1_data ||
        sizeof(SERVICE_PROCESSOR_BL1_DATA_t) != bl1_data->service_processor_bl1_data_size ||
        SERVICE_PROCESSOR_BL1_DATA_VERSION != bl1_data->service_processor_bl1_version) {
        printf("Invalid BL1 DATA!\n");
        return -1;
    }

    g_service_processor_bl2_data.service_processor_rom_version =
        bl1_data->service_processor_rom_version;
    g_service_processor_bl2_data.service_processor_bl1_version =
        bl1_data->service_processor_bl1_version;
    g_service_processor_bl2_data.sp_gpio_pins = bl1_data->sp_gpio_pins;
    g_service_processor_bl2_data.vaultip_coid_set = bl1_data->vaultip_coid_set;
    g_service_processor_bl2_data.spi_controller_rx_baudrate_divider =
        bl1_data->spi_controller_rx_baudrate_divider;
    g_service_processor_bl2_data.spi_controller_tx_baudrate_divider =
        bl1_data->spi_controller_tx_baudrate_divider;

    // copy major,minor,revision info of BL1 image
    g_service_processor_bl2_data.service_processor_bl1_image_file_version_major =
         bl1_data->service_processor_bl1_image_file_version_major;
    g_service_processor_bl2_data.service_processor_bl1_image_file_version_minor =
         bl1_data->service_processor_bl1_image_file_version_minor;
    g_service_processor_bl2_data.service_processor_bl1_image_file_version_revision =
         bl1_data->service_processor_bl1_image_file_version_revision;

    // copy the SP ROOT/ISSUING CA certificates chain
    memcpy(&(g_service_processor_bl2_data.sp_certificates), &(bl1_data->sp_certificates),
           sizeof(bl1_data->sp_certificates));

    // copy the SP BL1 header
    memcpy(&(g_service_processor_bl2_data.sp_bl1_header), &(bl1_data->sp_bl1_header),
           sizeof(bl1_data->sp_bl1_header));

    // copy the SP BL2 header
    memcpy(&(g_service_processor_bl2_data.sp_bl2_header), &(bl1_data->sp_bl2_header),
           sizeof(bl1_data->sp_bl2_header));

    return 0;
}

void bl2_main(const SERVICE_PROCESSOR_BL1_DATA_t *bl1_data);

void bl2_main(const SERVICE_PROCESSOR_BL1_DATA_t *bl1_data)
{
    bool vaultip_disabled;
    const IMAGE_VERSION_INFO_t *image_version_info = get_image_version_info();

    // Disable buffering on stdout
    setbuf(stdout, NULL);

    // In fast-boot mode, generate fake BL1-data (we are not running BL1)
#if FAST_BOOT
    static SERVICE_PROCESSOR_BL1_DATA_t fake_bl1_data;
    fake_bl1_data.service_processor_bl1_data_size = sizeof(fake_bl1_data);
    fake_bl1_data.service_processor_bl1_version = SERVICE_PROCESSOR_BL1_DATA_VERSION;
    fake_bl1_data.service_processor_rom_version = 0xDEADBEEF;
    fake_bl1_data.sp_gpio_pins = 0;
    fake_bl1_data.sp_pll0_frequency = 0;
    fake_bl1_data.sp_pll1_frequency = 0;
    fake_bl1_data.pcie_pll0_frequency = 0;
    fake_bl1_data.timer_raw_ticks_before_pll_turned_on = 0;
    fake_bl1_data.vaultip_coid_set = 0;
    fake_bl1_data.spi_controller_rx_baudrate_divider = 0;
    fake_bl1_data.spi_controller_tx_baudrate_divider = 0;
    // fake_bl1_data.flash_fs_bl1_info bypassed
    // fake_bl1_data.pcie_config_header bypassed
    // fake_bl1_data.sp_certificates[2] bypassed
    // fake_bl1_data.sp_bl1_header bypassed
    // fake_bl1_data.sp_bl2_header bypassed
    bl1_data = &fake_bl1_data;
#endif

    // In non-fast-boot mode, the bootrom initializes SPIO UART0
#if FAST_BOOT
    SERIAL_init(UART0);
#endif

    printf("\n** SP BL2 STARTED **\r\n");
    printf("BL2 version: %u.%u.%u:" GIT_VERSION_STRING " (" BL2_VARIANT ")\n",
           image_version_info->file_version_major, image_version_info->file_version_minor,
           image_version_info->file_version_revision);
    // printf("GIT hash: " GIT_HASH_STRING "\n");

    memset(&g_service_processor_bl2_data, 0, sizeof(g_service_processor_bl2_data));
    g_service_processor_bl2_data.service_processor_bl2_data_size =
        sizeof(g_service_processor_bl2_data);
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

    if (0 != pll_init(bl1_data->sp_pll0_frequency, bl1_data->sp_pll1_frequency,
                      bl1_data->pcie_pll0_frequency)) {
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
    if (0 != flashfs_drv_init(&g_service_processor_bl2_data.flash_fs_bl2_info,
                              &bl1_data->flash_fs_bl1_info)) {
        printf("flashfs_drv_init() failed!\n");
        goto FATAL_ERROR;
    }
#endif

    printf("Starting RTOS...\n");

    gs_taskHandleMain = xTaskCreateStatic(taskMain, "Main Task", TASK_STACK_SIZE, NULL, 1,
                                          gs_stackMain, &gs_taskBufferMain);
    if (gs_taskHandleMain == NULL) {
        printf("xTaskCreateStatic(taskMain) failed!\r\n");
    }

    vTaskStartScheduler();

FATAL_ERROR:
    printf("Encountered a FATAL ERROR!\n");
    printf("Waiting for RESET!!!\n");
    for (;;)
        ;
}

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
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

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
}
