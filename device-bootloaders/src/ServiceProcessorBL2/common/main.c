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

#include "minion_configuration.h"
#include "mem_controller.h"

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

#include "trace.h"
#include "log.h"

#if TEST_FRAMEWORK
#include "tf.h"
#endif

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

static int32_t configure_noc(void)
{
    /* Configure NOC to 400 Mhz */
    if (0 != configure_sp_pll_2(5)) {
        Log_Write(LOG_LEVEL_ERROR, "configure_sp_pll_2() failed!\n");
        return NOC_MAIN_CLOCK_CONFIGURE_ERROR;
    }

   // TBD: Configure Main NOC Registers via Regbus
   // Remap NOC ID based on MM OTP value
   // Other potential NOC workarounds

   return SUCCESS;
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
    /* Configure Pshire Pll to 1010 Mhz */
    configure_pshire_pll(6);
    PCIe_init(false /*expect_link_up*/);
#else
    PCIe_init(true /*expect_link_up*/);
#endif

    minion_shires_mask = Get_Active_Compute_Minion_Mask();

    Minion_State_Init(minion_shires_mask);

    // Create and Initialize all VQ for SP interfaces (Host and MM)
    sp_intf_init();

    /* Initialize the DIRs */
    DIR_Init();

    /* Set the minion shires available */
    DIR_Set_Minion_Shires(minion_shires_mask);

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_VQ_READY);

    Log_Write(LOG_LEVEL_INFO, "time: %lu\n", timer_get_ticks_count());

   // Setup NOC

    if (0 != configure_noc()) {
        Log_Write(LOG_LEVEL_ERROR, "configure_noc() failed!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_NOC_INITIALIZED);

   // Setup MemShire/DDR

    if (0 != configure_memshire()) {
        Log_Write(LOG_LEVEL_ERROR, "configure_memshire() failed!\n");
        goto FIRMWARE_LOAD_ERROR;
    }

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_DDR_INITIALIZED);

   // Setup Minions

    if (0 != Enable_Compute_Minion(minion_shires_mask)) {
        Log_Write(LOG_LEVEL_ERROR, "Enable Compute Minion failed!\n");
        goto FIRMWARE_LOAD_ERROR;
    }

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_MINION_INITIALIZED);

    // Minion FW Authenticate and load to DDR
    if (0 != Load_Autheticate_Minion_Firmware()) {
        Log_Write(LOG_LEVEL_ERROR, "Failed to load Minion Firmware!\n");
        goto FIRMWARE_LOAD_ERROR;
    }

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_MINION_FW_AUTHENTICATED_INITIALIZED);

    // Launch Dispatcher

    launch_command_dispatcher();

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_COMMAND_DISPATCHER_INITIALIZED);

    if (0 != Enable_Minion_Neighborhoods(minion_shires_mask)) {
        Log_Write(LOG_LEVEL_ERROR, "Failed to enable minion neighborhoods!\n");
        goto FIRMWARE_LOAD_ERROR;
    }

    // TODO: SW-3877 need to READ Master Shire ID from OTP, potentially reprogram NOC
    if (0 != Enable_Master_Shire_Threads(32)) {
        Log_Write(LOG_LEVEL_ERROR, "Failed to enable Master minion threads!\n");
        goto FIRMWARE_LOAD_ERROR;
    }

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_MM_FW_LAUNCHED);

    // Program ATUs here
    pcie_enable_link();

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_ATU_PROGRAMMED);

    // init thermal power management service
    if (0 != init_thermal_pwr_mgmt_service()) {
        Log_Write(LOG_LEVEL_ERROR, "Failed to init thermal power management!\n");
        goto FIRMWARE_LOAD_ERROR;
    }

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_PM_READY);

    // init watchdog service
     if (0 != init_watchdog_service()) {
        Log_Write(LOG_LEVEL_ERROR, "Failed to init watchdog service!\n");
        goto FIRMWARE_LOAD_ERROR;
    }

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_SP_WATCHDOG_TASK_READY);

    // init DM event handler task
    if (0 != dm_event_control_init()) {
        Log_Write(LOG_LEVEL_ERROR, "Failed to create dm event handler task!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_EVENT_HANDLER_READY);

#if !FAST_BOOT
    // SP and minions have booted successfully. Increment the completed boot counter
    if (0 != flashfs_drv_increment_completed_boot_count()) {
        Log_Write(LOG_LEVEL_ERROR, "Failed to increment the completed boot counter!\n");
        goto FIRMWARE_LOAD_ERROR;
    }
    Log_Write(LOG_LEVEL_INFO, "Incremented the completed boot counter!\n");
#endif

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_DEV_READY);
    Log_Write(LOG_LEVEL_INFO, "SP Device Ready!\n");


    // Init DM sampling task
    init_dm_sampling_task();

    goto DONE;

FIRMWARE_LOAD_ERROR:
    Log_Write(LOG_LEVEL_ERROR, "Fatal error... waiting for reset!\n");

DONE:
    while (1) {
        Log_Write(LOG_LEVEL_INFO, "M");
        vTaskDelay(2U);
    }
}

static int copy_bl1_data(const SERVICE_PROCESSOR_BL1_DATA_t *bl1_data)
{
    if (NULL == bl1_data ||
        sizeof(SERVICE_PROCESSOR_BL1_DATA_t) != bl1_data->service_processor_bl1_data_size ||
        SERVICE_PROCESSOR_BL1_DATA_VERSION != bl1_data->service_processor_bl1_version) {
        Log_Write(LOG_LEVEL_ERROR, "Invalid BL1 DATA!\n");
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

    // In Production mode, the bootrom initializes SPIO UART0
#if FAST_BOOT || TEST_FRAMEWORK
    SERIAL_init(UART0);
#endif

    SERIAL_init(UART1);
    SERIAL_init(PU_UART0);
    SERIAL_init(PU_UART1);

    Log_Init(LOG_LEVEL_INFO);
    Trace_Init_SP(NULL);

    Log_Write(LOG_LEVEL_INFO, "** SP BL2 STARTED **\n");
    Log_Write(LOG_LEVEL_INFO, "BL2 version: %u.%u.%u:" GIT_VERSION_STRING " (" BL2_VARIANT ")\n",
           image_version_info->file_version_major, image_version_info->file_version_minor,
           image_version_info->file_version_revision);

#if TEST_FRAMEWORK
    /* control does not return from call below for now .. */
    TF_Wait_And_Process_TF_Cmds();
#endif

    memset(&g_service_processor_bl2_data, 0, sizeof(g_service_processor_bl2_data));
    g_service_processor_bl2_data.service_processor_bl2_data_size =
        sizeof(g_service_processor_bl2_data);
    g_service_processor_bl2_data.service_processor_bl2_version = SERVICE_PROCESSOR_BL2_DATA_VERSION;


    if (0 != copy_bl1_data(bl1_data)) {
        Log_Write(LOG_LEVEL_ERROR, "copy_bl1_data() failed!!\n");
        goto FATAL_ERROR;
    }

    timer_init(bl1_data->timer_raw_ticks_before_pll_turned_on, bl1_data->sp_pll0_frequency);

    if (0 != sp_otp_init()) {
        Log_Write(LOG_LEVEL_ERROR, "sp_otp_init() failed!\n");
        goto FATAL_ERROR;
    }

    vaultip_disabled = is_vaultip_disabled();

    if (0 != pll_init(bl1_data->sp_pll0_frequency, bl1_data->sp_pll1_frequency,
                      bl1_data->pcie_pll0_frequency)) {
        Log_Write(LOG_LEVEL_ERROR, "pll_init() failed!\n");
        goto FATAL_ERROR;
    }

    INT_init();

    if (vaultip_disabled) {
        Log_Write(LOG_LEVEL_INFO, "VaultIP is disabled!\n");
    } else {
        if (0 != Vault_Initialize()) {
            Log_Write(LOG_LEVEL_ERROR, "Vault_Initialize() failed!\n");
            goto FATAL_ERROR;
        }
        if (0 != crypto_init(bl1_data->vaultip_coid_set)) {
            Log_Write(LOG_LEVEL_ERROR, "crypto_init() failed!\n");
            goto FATAL_ERROR;
        }
    }

    // In fast-boot mode we skip loading from flash, and assume everything is already pre-loaded
#if !FAST_BOOT
    if (0 != flashfs_drv_init(&g_service_processor_bl2_data.flash_fs_bl2_info,
                              &bl1_data->flash_fs_bl1_info)) {
        Log_Write(LOG_LEVEL_ERROR, "flashfs_drv_init() failed!\n");
        goto FATAL_ERROR;
    }
#endif

    Log_Write(LOG_LEVEL_INFO, "Starting RTOS...\n");

    gs_taskHandleMain = xTaskCreateStatic(taskMain, "Main Task", TASK_STACK_SIZE, NULL, 1,
                                          gs_stackMain, &gs_taskBufferMain);
    if (gs_taskHandleMain == NULL) {
        Log_Write(LOG_LEVEL_ERROR, "xTaskCreateStatic(taskMain) failed!\r\n");
    }

    vTaskStartScheduler();

FATAL_ERROR:
    Log_Write(LOG_LEVEL_ERROR, "Encountered a FATAL ERROR!\n");
    Log_Write(LOG_LEVEL_ERROR, "Waiting for RESET!!!\n");
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
