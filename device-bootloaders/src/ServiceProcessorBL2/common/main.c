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

#include "noc_configuration.h"
#include "minion_configuration.h"
#include "mem_controller.h"

#include "hwinc/sp_cru_reset.h"
#include "hwinc/hal_device.h"

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

/*! \def ASSERT_FATAL(cond, log)
    \brief A blocking assertion macro with serial log for SP runtime
*/
#define ASSERT_FATAL(cond, log)                                                    \
    if (!(cond))                                                                   \
    {                                                                              \
        Log_Write(LOG_LEVEL_ERROR,                                                 \
        "Fatal error on line %d in %s: %s\r\n", __LINE__, __FUNCTION__, log);      \
        while (1)                                                                  \
        {                                                                          \
            Log_Write(LOG_LEVEL_CRITICAL, "M");                                    \
            vTaskDelay(2U);                                                        \
        }                                                                          \
    }

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

static TaskHandle_t gs_taskHandleMain;
static StackType_t gs_stackMain[TASK_STACK_SIZE];
static StaticTask_t gs_taskBufferMain;

static void taskMain(void *pvParameters)
{
    uint64_t minion_shires_mask;
    uint8_t mm_id;
    int status;
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
    #if !TEST_FRAMEWORK /* Do not initialize PCIe for TF */
    PCIe_init(true /*expect_link_up*/);
    #endif
#endif

    // Initialize Compute Minions based on active compute minion mask
    minion_shires_mask = Minion_Get_Active_Compute_Minion_Mask();

    Minion_State_Init(minion_shires_mask);

    // Initialize Host to Service Processor Interface
    #if !TEST_FRAMEWORK
    status = SP_Host_Iface_Init();
    ASSERT_FATAL(status == STATUS_SUCCESS, "SP Host Interface Initialization failed!")
    #endif

    // Initialize Service Processor to Master Minion FW interface
    status = MM_Iface_Init();
    ASSERT_FATAL(status == STATUS_SUCCESS, "SP to Master Minion FW Interface Initialization failed!")

    /* Initialize the DIRs */
    DIR_Init();

    /* Set the minion shires available */
    DIR_Set_Minion_Shires(minion_shires_mask);
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_VQ_READY);
    Log_Write(LOG_LEVEL_CRITICAL, "time: %lu\n", timer_get_ticks_count());

    // Setup NOC
    status = NOC_Configure(5); /* Configure NOC to 400 Mhz */
    ASSERT_FATAL(status == STATUS_SUCCESS, "configure_noc() failed!")
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_NOC_INITIALIZED);

    // Setup MemShire/DDR
    status = configure_memshire();
    ASSERT_FATAL(status == STATUS_SUCCESS, "configure_memshire() failed!")
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_DDR_INITIALIZED);

    // Setup Minions
    /* Setup default PLL to be 650 Mhz (Mode -3) */
    status = Minion_Enable_Compute_Minion(minion_shires_mask, 3);
    ASSERT_FATAL(status == STATUS_SUCCESS, "Enable Compute Minion failed!")

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_MINION_INITIALIZED);

#if !TEST_FRAMEWORK
    // Minion FW Authenticate and load to DDR
    status = Minion_Load_Authenticate_Firmware();
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed to load Minion Firmware!")
#endif
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_MINION_FW_AUTHENTICATED_INITIALIZED);

    // Launch Dispatcher
#if !TEST_FRAMEWORK
    launch_host_sp_command_handler();
#endif
    launch_mm_sp_command_handler();

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_COMMAND_DISPATCHER_INITIALIZED);

    status = Minion_Enable_Neighborhoods(minion_shires_mask);
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed to enable minion neighborhoods!")

    // Potentially reprogram NOC?
    status = otp_get_master_shire_id(&mm_id);
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed to read master minion shire ID!")

    //Value wasn't burned into fuse, using default value
    if (mm_id == 0xff){
       Log_Write(LOG_LEVEL_WARNING, "Master shire ID was not found in the OTP, using default value of 32!\n");
       mm_id = 32;
    }

    status = Minion_Enable_Master_Shire_Threads(mm_id);
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed to enable Master minion threads!")

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_MM_FW_LAUNCHED);

#if (TEST_FRAMEWORK)
    Log_Write(LOG_LEVEL_INFO, "TF SW bring up ...\r\n");
    /* Control does not return from call below for now .. */
    TF_Wait_And_Process_TF_Cmds();
#endif

    // Program ATUs here
    pcie_enable_link();

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_ATU_PROGRAMMED);

    // init thermal power management service
    status = init_thermal_pwr_mgmt_service();
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed to init thermal power management!")
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_PM_READY);

    // init watchdog service
    status = init_watchdog_service();
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed to init watchdog service!")
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_SP_WATCHDOG_TASK_READY);

    // init DM event handler task
    status = dm_event_control_init();
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed to create dm event handler task!")
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_EVENT_HANDLER_READY);

#if !FAST_BOOT
    // SP and minions have booted successfully. Increment the completed boot counter
    status = flashfs_drv_increment_completed_boot_count();
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed to increment the completed boot counter!")
    Log_Write(LOG_LEVEL_CRITICAL, "Incremented the completed boot counter!\n");
#endif

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_DEV_READY);
    Log_Write(LOG_LEVEL_CRITICAL, "SP Device Ready!\n");

    // Init DM sampling task
    init_dm_sampling_task();

    /* Redirect the log messages to trace buffer after initialization is done */
    Log_Set_Interface(LOG_DUMP_TO_TRACE);

    while (1) {
        Log_Write(LOG_LEVEL_CRITICAL, "M");
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
#if FAST_BOOT || TEST_FRAMEWORK
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

    Log_Init(LOG_LEVEL_WARNING);
    Trace_Init_SP(NULL);
    Log_Write(LOG_LEVEL_CRITICAL, "\n** SP BL2 STARTED **\r\n");
    Log_Write(LOG_LEVEL_CRITICAL, "BL2 version: %u.%u.%u:" GIT_VERSION_STRING " (" BL2_VARIANT ")\n",
           image_version_info->file_version_major, image_version_info->file_version_minor,
           image_version_info->file_version_revision);

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
#if !(FAST_BOOT || TEST_FRAMEWORK)
    if (0 != flashfs_drv_init(&g_service_processor_bl2_data.flash_fs_bl2_info,
                              &bl1_data->flash_fs_bl1_info)) {
        Log_Write(LOG_LEVEL_ERROR, "flashfs_drv_init() failed!\n");
        goto FATAL_ERROR;
    }
#endif

    Log_Write(LOG_LEVEL_CRITICAL, "Starting RTOS...\n");

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
