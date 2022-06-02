#include "config/mgmt_build_config.h"
#include "etsoc/drivers/serial/serial.h"
#include "interrupt.h"
#include "dummy_isr.h"
#include "etsoc/drivers/pcie/pcie_int.h"

#include "FreeRTOS.h"
#include "task.h"

#include "cache_flush_ops.h"
#include "etsoc/isa/io.h"
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
#include "bl2_pvt_controller.h"

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
#define ASSERT_FATAL(cond, log)                                                                    \
    if (!(cond))                                                                                   \
    {                                                                                              \
        Log_Write(LOG_LEVEL_ERROR, "Fatal error on line %d in %s: %s\r\n", __LINE__, __FUNCTION__, \
                  log);                                                                            \
        Log_Write(LOG_LEVEL_ERROR, "Waiting for SOC RESET!!!\n");                                  \
        while (1)                                                                                  \
        {                                                                                          \
            Log_Write(LOG_LEVEL_CRITICAL, "SP Down..");                                            \
            vTaskDelay(1000U);                                                                     \
        }                                                                                          \
    }

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize);

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize);
void vApplicationIdleHook(void);
void vApplicationTickHook(void);
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);

static SERVICE_PROCESSOR_BL2_DATA_t g_service_processor_bl2_data;

/* NOC frequency modes (500MHz) for different ref clocks, 100MHz, 24Mhz and 40MHz */
static uint8_t noc_pll_mode[3] = { 5, 11, 17 };

/* MIN STEP CLOCK frequency modes (650MHz) for different ref clocks, 100MHz, 24Mhz and 40MHz */
static uint8_t min_step_pll_mode[3] = { 44, 45, 46 };

/* LVDPLL frequency modes (500MHz) for different ref clocks, 100MHz, 24Mhz and 40MHz */
static uint8_t min_lvdpll_mode[3] = { 15, 60, 105 };

SERVICE_PROCESSOR_BL2_DATA_t *get_service_processor_bl2_data(void)
{
    return &g_service_processor_bl2_data;
}

static TaskHandle_t gs_taskHandleMain;
static StackType_t gs_stackMain[MAIN_TASK_STACK_SIZE];
static StaticTask_t gs_taskBufferMain;

static void taskMain(void *pvParameters)
{
    uint64_t minion_shires_mask;
    int status;
    (void)pvParameters;

    // Disable buffering on stdout
    setbuf(stdout, NULL);

    log_level_t log_level;
    log_level = Log_Get_Level();
    Log_Set_Level(log_level);

    // Establish connection to PMIC
    // NOSONAR struct module_voltage_t module_voltage ={0};
    setup_pmic();
    // Read all Voltage Rails
    // Bug in PMIC FW preventing access to these registers
    // NOSONAR get_module_voltage(&module_voltage);

    // Read and printout ecid
    ecid_t ecid;
    read_ecid(&ecid);

#if FAST_BOOT
    // In cases where BootROM is bypass, initialize PCIe link
    PCIe_release_pshire_from_reset();
    /* Configure Pshire Pll to 1010 Mhz */
    configure_pshire_pll(6);
    PCIe_init(false /*expect_link_down*/);
#else
#if !TEST_FRAMEWORK
    // If not built for TF initialize PCIe
    PCIe_init(true /*expect_link_up*/);
#else
    // If build is for TF skip PCIe initilize, except if
    // TF_Entry_Point is set to TF_BL2_ENTRY_FOR_DM_WITH_PCIE
    if (TF_Get_Entry_Point() == TF_BL2_ENTRY_FOR_DM_WITH_PCIE)
    {
        PCIe_init(true /*expect_link_up*/);
    }
#endif
#endif

    // Extract the active Compute Minions based on fuse
    minion_shires_mask = Minion_Get_Active_Compute_Minion_Mask();
    Minion_Set_Active_Shire_Mask(minion_shires_mask);

    // Initialize Minions
    Log_Write(LOG_LEVEL_CRITICAL, "MAIN:[txt]Initialize Minion Shire\n");
    status = Initialize_Minions(minion_shires_mask);
    ASSERT_FATAL(status == STATUS_SUCCESS, "Minion initialization failed!")

    // Initialize Host to Service Processor Interface
#if !TEST_FRAMEWORK
    status = SP_Host_Iface_Init();
    ASSERT_FATAL(status == STATUS_SUCCESS, "SP Host Interface Initialization failed!")
#else
    // If build is for TF skip Host to SP interface initilization,
    // except if TF_Entry_Point is set to TF_BL2_ENTRY_FOR_DM_WITH_PCIE
    if (TF_Get_Entry_Point() == TF_BL2_ENTRY_FOR_DM_WITH_PCIE)
    {
        // Initialize Host->SP interface
        status = SP_Host_Iface_Init();
        ASSERT_FATAL(status == STATUS_SUCCESS, "SP Host Interface Initialization failed!")
        // Launch Host->SP Command Handler
        launch_host_sp_command_handler();
    }
#endif

    // Initialize Service Processor to Master Minion FW interface
    status = MM_Iface_Init();
    ASSERT_FATAL(status == STATUS_SUCCESS,
                 "SP to Master Minion FW Interface Initialization failed!")

    /* Initialize the DIRs */
    DIR_Init();
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_VQ_READY);

    Log_Write(LOG_LEVEL_CRITICAL, "time: %lu\n", timer_get_ticks_count());

    uint8_t hpdpll_strap_pins;
    hpdpll_strap_pins = get_hpdpll_strap_value();

    // Setup NOC
    Log_Write(LOG_LEVEL_CRITICAL, "MAIN:[txt]Configure NOC\n");
    status = NOC_Configure(noc_pll_mode[hpdpll_strap_pins]);
    ASSERT_FATAL(status == STATUS_SUCCESS, "configure_noc() failed!")
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_NOC_INITIALIZED);

#if !(FAST_BOOT || TEST_FRAMEWORK)
    // Initialize Flash service
    Log_Write(LOG_LEVEL_CRITICAL, "MAIN:[txt]Flash FileSystem ..");
    status = flashfs_drv_init();
    ASSERT_FATAL(status == STATUS_SUCCESS, "flashfs_drv_init() failed!")
#endif

    // TF Hook for testing of Device Management services only
    // or for Device Management Services with PCIe enabled
#if TEST_FRAMEWORK
    // Control does not return from call below
    // if TF_Interception_Point is set by host to TF_BL2_ENTRY_FOR_DM ..
    Log_Write(LOG_LEVEL_INFO, "Entering TF_BL2_ENTRY_FOR_DM intercept\r\n");
    TF_Wait_And_Process_TF_Cmds(TF_BL2_ENTRY_FOR_DM);
    Log_Write(LOG_LEVEL_INFO, "Fall thru TF_BL2_ENTRY_FOR_DM intercept\r\n");

    Log_Write(LOG_LEVEL_INFO, "Entering TF_BL2_ENTRY_FOR_DM_WITH_PCIE intercept\r\n");
    TF_Wait_And_Process_TF_Cmds(TF_BL2_ENTRY_FOR_DM_WITH_PCIE);
    Log_Write(LOG_LEVEL_INFO, "Fall thru TF_BL2_ENTRY_FOR_DM_WITH_PCIE intercept\r\n");
#endif

    // Setup MemShire/DDR
    Log_Write(LOG_LEVEL_CRITICAL, "MAIN:[txt]MemShire Configure\n");
    status = configure_memshire();
    ASSERT_FATAL(status == STATUS_SUCCESS, "configure_memshire() failed!")
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_DDR_INITIALIZED);

    /* Setup Compute Minions Shire Clocks and bring them out of Reset */
    uint8_t lvdpll_strap_pins;
    lvdpll_strap_pins = get_lvdpll_strap_value();

    Log_Write(LOG_LEVEL_CRITICAL, "MAIN:[txt]Minion Shire PLL Configure \n");
    Log_Write(LOG_LEVEL_INFO, "HDPLL[Strap: %d] mode: %d LVDPLL[Strap: %d] mode: %d\n",
              hpdpll_strap_pins, min_step_pll_mode[hpdpll_strap_pins], lvdpll_strap_pins,
              min_lvdpll_mode[lvdpll_strap_pins]);

    status = Minion_Configure_Minion_Shire_PLL(minion_shires_mask,
                                               min_step_pll_mode[hpdpll_strap_pins],
                                               min_lvdpll_mode[lvdpll_strap_pins], true);
    ASSERT_FATAL(status == STATUS_SUCCESS, "Enable Compute Minion failed!")

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_MINION_INITIALIZED);

    // Initialize Vault service
    if (!is_vaultip_disabled())
    {
        status = Vault_Initialize();
        ASSERT_FATAL(status == STATUS_SUCCESS, "Vault_Initialize() failed!")
        status = crypto_init(get_service_processor_bl2_data()->vaultip_coid_set);
        ASSERT_FATAL(status == STATUS_SUCCESS, "crypto_init() failed!")
    }

#if !(FAST_BOOT || TEST_FRAMEWORK)
    // Extract Minion FW from Flash, authenticate and load to DDR
    status = Minion_Load_Authenticate_Firmware();
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed to load Minion Firmware!")
#endif

    DIR_Set_Service_Processor_Status(
        SP_DEV_INTF_SP_BOOT_STATUS_MINION_FW_AUTHENTICATED_INITIALIZED);

#if !TEST_FRAMEWORK
    // Launch Host->SP Command Handler
    launch_host_sp_command_handler();
#endif
    // Launch MM->SP Command Handler
    launch_mm_sp_command_handler();

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_COMMAND_DISPATCHER_INITIALIZED);

    /* Initialize the Cache Size DIRs */
    DIR_Cache_Size_Init();

    // Launch Master Minion Runtime
    Log_Write(LOG_LEVEL_INFO, "MAIN:[txt]Minion_Enable_Master_Shire_Threads\n");
    status = Minion_Enable_Master_Shire_Threads();
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed to enable Master minion threads!")

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_MM_FW_LAUNCHED);

#if TEST_FRAMEWORK
    Log_Write(LOG_LEVEL_INFO, "Entering TF_BL2_ENTRY_FOR_SP_MM intercept.\r\n");
    /* Control does not return from call below */
    /* if TF_Interception_Point is set by host to TF_BL2_ENTRY_FOR_SP_MM .. */
    TF_Wait_And_Process_TF_Cmds(TF_BL2_ENTRY_FOR_SP_MM);
    Log_Write(LOG_LEVEL_INFO, "Fall thru TF_BL2_ENTRY_FOR_SP_MM intercept\r\n");
#endif

    // Program ATUs here
    pcie_enable_link();

    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_ATU_PROGRAMMED);

    // Initialize  watchdog service
    status = init_watchdog_service();
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed to init watchdog service!")
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_SP_WATCHDOG_TASK_READY);

#if !FAST_BOOT
    // At this point, SP and minions have booted successfully. Increment the completed boot counter
    status = flash_fs_increment_completed_boot_count();
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed to increment the completed boot counter!")
    Log_Write(LOG_LEVEL_INFO, "Incremented the completed boot counter!\n");
#endif

    // Initialize thermal power management service
    status = init_thermal_pwr_mgmt_service();
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed to init thermal power management!")
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_PM_READY);

    /* Populate the device generic attributes */
    DIR_Generic_Attributes_Init();

    if (Minion_State_Get_MM_Heartbeat_Count() == 0)
    {
        /* Warn, MM is not ready */
        Log_Write(LOG_LEVEL_WARNING, "MM not ready, SP has not received MM heartbeat!\r\n");
    }
    else
    {
        /* Warn, MM heartbeat alive */
        Log_Write(LOG_LEVEL_CRITICAL, "MM heartbeat alive!\r\n");
    }

    // Initialize  DM event handler task
    status = dm_event_control_init();
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed to create dm event handler task!")
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_EVENT_HANDLER_READY);

    /* Print system operating point */
    print_system_operating_point();

    /* Inform Host Device is Ready */
    Log_Write(LOG_LEVEL_CRITICAL, "SP Device Ready!\r\n");
    DIR_Set_Service_Processor_Status(SP_DEV_INTF_SP_BOOT_STATUS_DEV_READY);

    /* Initialize SP Trace component */
    status = Trace_Init_SP(NULL);
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed to init SP trace component!")

    /* Initialize SP Exception Trace component */
    status = Trace_Exception_Init_SP(NULL);
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed to init SP Exception trace component!")

    /* Initialize Dev Stats Trace component */
    status = Trace_Init_SP_Dev_Stats(NULL);
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed to init Dev Stats trace component!")

    // Initialize DM sampling task
    Log_Write(LOG_LEVEL_INFO, "MAIN:[txt] DM Sampling Task Start\n");
    init_dm_sampling_task();

    /* Redirect the log messages to trace buffer after initialization is done */
    Log_Set_Interface(LOG_DUMP_TO_TRACE);

    while (1)
    {
        Log_Write(LOG_LEVEL_CRITICAL, "SP Alive..\r\n");
        // Print SP Hearbeat
        vTaskDelay(MAIN_DEFAULT_TIMEOUT_MSEC);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       copy_partition_info_data
*
*   DESCRIPTION
*
*       This function initialize bl2 partition info struct by copying data
*       from bl1 partition info struct
*
*   INPUTS
*
*       bl1_partition_info   bl1 partition info struct
*
*   OUTPUTS
*
*       bl2_partition_info   bl2 partition info struct
*
***********************************************************************/
static int copy_partition_info_data(ESPERANTO_PARTITION_BL2_INFO_t *bl2_partition_info,
                                    const ESPERANTO_PARTITION_BL1_INFO_t *bl1_partition_info)
{
    if (NULL == bl2_partition_info || NULL == bl1_partition_info)
    {
        Log_Write(LOG_LEVEL_ERROR, "copy_partition_info_data: invalid arguments!\n");
        return -1;
    }

    memcpy(&(bl2_partition_info->header), &(bl1_partition_info->header),
           sizeof(bl1_partition_info->header));
    memcpy(&(bl2_partition_info->regions_table), &(bl1_partition_info->regions_table),
           sizeof(bl1_partition_info->regions_table));

    bl2_partition_info->priority_designator_region_index =
        bl1_partition_info->priority_designator_region_index;
    bl2_partition_info->boot_counters_region_index = bl1_partition_info->boot_counters_region_index;
    bl2_partition_info->configuration_data_region_index =
        bl1_partition_info->configuration_data_region_index;
    bl2_partition_info->vaultip_fw_region_index = bl1_partition_info->vaultip_fw_region_index;
    bl2_partition_info->pcie_config_region_index = bl1_partition_info->pcie_config_region_index;
    bl2_partition_info->sp_certificates_region_index =
        bl1_partition_info->sp_certificates_region_index;
    bl2_partition_info->sp_bl1_region_index = bl1_partition_info->sp_bl1_region_index;
    bl2_partition_info->sp_bl2_region_index = bl1_partition_info->sp_bl2_region_index;

    memcpy(&(bl2_partition_info->priority_designator_region_data),
           &(bl1_partition_info->priority_designator_region_data),
           sizeof(bl1_partition_info->priority_designator_region_data));
    memcpy(&(bl2_partition_info->boot_counters_region_data),
           &(bl1_partition_info->boot_counters_region_data),
           sizeof(bl1_partition_info->boot_counters_region_data));

    bl2_partition_info->priority_counter = bl1_partition_info->priority_counter;
    bl2_partition_info->attempted_boot_counter = bl1_partition_info->attempted_boot_counter;
    bl2_partition_info->completed_boot_counter = bl1_partition_info->completed_boot_counter;
    bl2_partition_info->partition_valid = bl1_partition_info->partition_valid;

    return 0;
}

static int copy_flash_fs_data(FLASH_FS_BL2_INFO_t *flash_fs_bl2_info,
                              const FLASH_FS_BL1_INFO_t *flash_fs_bl1_info)
{
    if (NULL == flash_fs_bl2_info || NULL == flash_fs_bl1_info)
    {
        Log_Write(LOG_LEVEL_ERROR, "copy_flash_fs_data: invalid arguments!\n");
        return -1;
    }

    memset(flash_fs_bl2_info, 0, sizeof(FLASH_FS_BL2_INFO_t));

    for (uint32_t n = 0; n < 2; n++)
    {
        if (0 != copy_partition_info_data(&(flash_fs_bl2_info->partition_info[n]),
                                          &(flash_fs_bl1_info->partition_info[n])))
        {
            Log_Write(LOG_LEVEL_ERROR, "copy_partition_info_data(%u) failed!\n", n);
            return -1;
        }
    }

    flash_fs_bl2_info->flash_id_u32 = flash_fs_bl1_info->flash_id_u32;
    flash_fs_bl2_info->flash_size = flash_fs_bl1_info->flash_size;
    flash_fs_bl2_info->active_partition = flash_fs_bl1_info->active_partition;
    flash_fs_bl2_info->other_partition_valid = flash_fs_bl1_info->other_partition_valid;
    flash_fs_bl2_info->configuration_region_address =
        flash_fs_bl1_info->configuration_region_address;
    flash_fs_bl2_info->pcie_config_file_info = flash_fs_bl1_info->pcie_config_file_info;
    flash_fs_bl2_info->vaultip_firmware_file_info = flash_fs_bl1_info->vaultip_firmware_file_info;
    flash_fs_bl2_info->sp_certificates_file_info = flash_fs_bl1_info->sp_certificates_file_info;
    flash_fs_bl2_info->sp_bl1_file_info = flash_fs_bl1_info->sp_bl1_file_info;
    flash_fs_bl2_info->sp_bl2_file_info = flash_fs_bl1_info->sp_bl2_file_info;

    return 0;
}

static int initialize_bl2_data(const SERVICE_PROCESSOR_BL1_DATA_t *bl1_data)
{
    if (NULL == bl1_data ||
        sizeof(SERVICE_PROCESSOR_BL1_DATA_t) != bl1_data->service_processor_bl1_data_size ||
        SERVICE_PROCESSOR_BL1_DATA_VERSION != bl1_data->service_processor_bl1_version)
    {
        Log_Write(LOG_LEVEL_ERROR, "Invalid BL1 DATA!\n");
        return -1;
    }

    memset(&g_service_processor_bl2_data, 0, sizeof(g_service_processor_bl2_data));
    g_service_processor_bl2_data.service_processor_bl2_data_size =
        sizeof(g_service_processor_bl2_data);
    g_service_processor_bl2_data.service_processor_bl2_version = SERVICE_PROCESSOR_BL2_DATA_VERSION;

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

    // Initialize BL2 Flash File-System by copy content from BL1 Flash info
    if (0 != copy_flash_fs_data(&g_service_processor_bl2_data.flash_fs_bl2_info,
                                &bl1_data->flash_fs_bl1_info))
    {
        Log_Write(LOG_LEVEL_ERROR, "copy_flash_fs_data() failed!\n");
        return -1;
    }

    return 0;
}

void bl2_main(const SERVICE_PROCESSOR_BL1_DATA_t *bl1_data);

void bl2_main(const SERVICE_PROCESSOR_BL1_DATA_t *bl1_data)
{
    int status;

    // Populate BL2 Image information
    const IMAGE_VERSION_INFO_t *image_version_info = get_image_version_info();

    // Disable buffering on stdout
    setbuf(stdout, NULL);

#if FAST_BOOT || TEST_FRAMEWORK

    // In cases where BL1 is skipped, generate fake BL1-data
    static SERVICE_PROCESSOR_BL1_DATA_t fake_bl1_data;
    fake_bl1_data.service_processor_bl1_data_size = sizeof(fake_bl1_data);
    fake_bl1_data.service_processor_bl1_version = SERVICE_PROCESSOR_BL1_DATA_VERSION;
    fake_bl1_data.service_processor_rom_version = 0xDEADBEEF;
    fake_bl1_data.sp_gpio_pins = 0;
    fake_bl1_data.sp_pll0_frequency = 1000;
    fake_bl1_data.sp_pll1_frequency = 2000;
    fake_bl1_data.pcie_pll0_frequency = 1010;
    fake_bl1_data.timer_raw_ticks_before_pll_turned_on = 0;
    fake_bl1_data.vaultip_coid_set = 0;
    fake_bl1_data.spi_controller_rx_baudrate_divider = 14;
    fake_bl1_data.spi_controller_tx_baudrate_divider = 14;
    fake_bl1_data.flash_fs_bl1_info.flash_id_u32 = SPI_FLASH_ON_PACKAGE;
    fake_bl1_data.flash_fs_bl1_info.flash_size = 0x100000;
    fake_bl1_data.flash_fs_bl1_info.active_partition = 0;
    fake_bl1_data.flash_fs_bl1_info.partition_info[fake_bl1_data.flash_fs_bl1_info.active_partition]
        .partition_valid = true;

    bl1_data = &fake_bl1_data;

    // In cases where production BootROM is skipped, initializes SPIO UART0
    SERIAL_init(SP_UART0);
#endif

    // Initialize all UART and Trace support
    SERIAL_init(SP_UART1);
    SERIAL_init(PU_UART0);
    SERIAL_init(PU_UART1);

    Log_Init(LOG_LEVEL_WARNING);

    /* Initialize and start continuous sampling on PVT sensors */
    pvt_init();

#if TEST_FRAMEWORK
    // Control does not return from call below
    // if TF_Interception_Point is set by host to TF_BL2_ENTRY_FOR_HW ..
    Trace_Init_SP(NULL);
    Log_Set_Level(LOG_LEVEL_INFO);
    Log_Write(
        LOG_LEVEL_INFO,
        "Entering TF_DEFAULT_ENTRY intercept. Waiting for host to set interception point.\r\n");
    TF_Wait_And_Process_TF_Cmds(TF_DEFAULT_ENTRY);
    Log_Write(LOG_LEVEL_INFO, "Host set TFintercept to: %d.\n", TF_Get_Entry_Point());

    Log_Write(LOG_LEVEL_INFO, "Entering TF_BL2_ENTRY_FOR_HW intercept.\r\n");
    TF_Wait_And_Process_TF_Cmds(TF_BL2_ENTRY_FOR_HW);
    Log_Write(LOG_LEVEL_INFO, "Fall thru TF_BL2_ENTRY_FOR_HW intercept.\r\n");
#endif

    // Initialize Splash screen
    Log_Write(LOG_LEVEL_CRITICAL, "\n** SP BL2 STARTED **\r\n");
    Log_Write(LOG_LEVEL_CRITICAL,
              "BL2 version: %u.%u.%u:" GIT_VERSION_STRING " (" BL2_VARIANT ")\n",
              image_version_info->file_version_major, image_version_info->file_version_minor,
              image_version_info->file_version_revision);

    // Populate BL2 globals using BL1 data from previous BL1
    status = initialize_bl2_data(bl1_data);
    ASSERT_FATAL(status == STATUS_SUCCESS, "Failed initialize_bl2_data()")

    // Start System Timer
    timer_init(bl1_data->timer_raw_ticks_before_pll_turned_on, bl1_data->sp_pll0_frequency);

    // Initialize SP OTP
    status = sp_otp_init();
    ASSERT_FATAL(status == STATUS_SUCCESS, "sp_otp_init() failed!")

    // Initialize PLL 0,1, PShire globals
    status = pll_init(bl1_data->sp_pll0_frequency, bl1_data->sp_pll1_frequency,
                      bl1_data->pcie_pll0_frequency);
    ASSERT_FATAL(status == STATUS_SUCCESS, "pll_init() failed!")

    // Initialize System Interrupt
    INT_init();

    // Initialize SPI controller
    status = SPI_Flash_Initialize(g_service_processor_bl2_data.flash_fs_bl2_info.flash_id);
    ASSERT_FATAL(status == STATUS_SUCCESS, "SPI_Flash_Initialize() failed!")

    // Initialize flash fs
    status = flash_fs_init(&g_service_processor_bl2_data.flash_fs_bl2_info);
    ASSERT_FATAL(status == STATUS_SUCCESS, "flash_fs_init() failed!")

    // Create Main RTOS task and launch Scheduler
    Log_Write(LOG_LEVEL_CRITICAL, "Starting RTOS...\n");
    gs_taskHandleMain = xTaskCreateStatic(taskMain, "Main Task", MAIN_TASK_STACK_SIZE, NULL, 1,
                                          gs_stackMain, &gs_taskBufferMain);
    ASSERT_FATAL(gs_taskHandleMain != NULL, "xTaskCreateStatic(taskMain) failed!")
    vTaskStartScheduler();

    // If Scheduler fails, spin waiting for SOC Reset
    ASSERT_FATAL(false, "Failed to Launch RTOS Scheduler!")
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

/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    /* If the buffers to be provided to the Timer task are declared inside this
    function then they must be declared static - otherwise they will be allocated on
    the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
    task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
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
