/************************************************************************
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
************************************************************************/
/*! \file minion_configuration.c
    \brief A C module that implements the minion shire configuration services.

    Public interfaces:
        Minion_Enable_Neighborhoods
        Minion_Enable_Master_Shire_Threads
        Minion_Shire_Update_Voltage
        Minion_Program_Step_Clock_PLL
        Minion_Enable_Compute_Minion
        Minion_Get_Active_Compute_Minion_Mask
        Minion_Load_Authenticate_Firmware
        Minion_Shire_Update_PLL_Freq
        Minion_Read_ESR
        Minion_Read_ESR
        Minion_Kernel_Launch
        Minion_State_Init
        Minion_State_MM_Iface_Get_Active_Shire_Mask
        Minion_State_Host_Iface_Process_Request
        Minion_State_MM_Error_Handler
        Minion_State_Error_Control_Init
        Minion_State_Error_Control_Deinit
        Minion_State_Set_Exception_Error_Threshold
        Minion_State_Set_Hang_Error_Threshold
        Minion_State_Get_Exception_Error_Count
        Minion_State_Get_Hang_Error_Count
*/
/***********************************************************************/
#include <stdio.h>

#include <hwinc/etsoc_shire_other_esr.h>

#include "esr.h"
#include "minion_configuration.h"

/*!
 * @struct struct minion_event_control_block
 * @brief Minion error event mgmt control block
 */
struct minion_event_control_block {
    uint32_t except_count;       /**< Exception error count. */
    uint32_t hang_count;         /**< Hang error count. */
    uint32_t except_threshold;   /**< Exception error count threshold. */
    uint32_t hang_threshold;     /**< Hang error count threshold. */
    uint64_t active_shire_mask;  /**< active shire number mask. */
    dm_event_isr_callback event_cb; /**< Event callback handler. */
};

/* The driver can populate this structure with the defaults that will be used during the init
    phase.*/

static struct minion_event_control_block event_control_block __attribute__((section(".data")));

/* This Threashold voltage would need to be extracted from OTP */
#define THRESHOLD_VOLTAGE 400

/* This chracterized co-efficient needs to be extracted from OTP */
#define DeltaVolt_per_DeltaFreq 0

// Define for Threads which participate in the Device Runtime FW management.
// Currently only lower 16 Minions (64 Harts) of the whole Minion Shire which
// participate in the Device Runtime. This might change with Virtual Queue
// implementation
#define MM_RT_THREADS 0x0000FFFFU

static uint64_t g_active_shire_mask = 0;

/*==================== Function Separator =============================*/

/*! \def TIMEOUT_PLL_CONFIG
    \brief PLL timeout configuration
*/
#define TIMEOUT_PLL_CONFIG 100000

/*! \def TIMEOUT_PLL_LOCK
    \brief lock timeout for PLL
*/
#define TIMEOUT_PLL_LOCK   100000

/*==================== Function Separator =============================*/

uint8_t pll_freq_to_mode(int32_t freq)
{
//NOSONAR TODO: Need to add Freq to mode convertion equation
   (void) freq;
   return 6; 
}

// Fixme: SW-8063  Replace with version from HAL
static int pll_config(uint8_t shire_id)
{
    uint64_t reg_value;

    /* Select 1 GHz from step_clock, Bits[2:0] = 3'b011. Bit 3 to '1' to go with DLL output */
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, 2, 
                    ETSOC_SHIRE_OTHER_ESR_SHIRE_CTRL_CLOCKMUX_ADDRESS, 0xb, 0);

    /* Auto-config register set dll_enable and get reset deasserted of the DLL */
    reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_DLL_AUTO_CONFIG_PCLK_SEL_SET(2) |
                ETSOC_SHIRE_OTHER_ESR_SHIRE_DLL_AUTO_CONFIG_DLL_ENABLE_SET(1);
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, 2, 
                    ETSOC_SHIRE_OTHER_ESR_SHIRE_DLL_AUTO_CONFIG_ADDRESS, reg_value, 0);

    /* Wait until DLL is locked to change clock mux */
    while (!(read_esr_new(PP_MACHINE, shire_id, REGION_OTHER, 2, 
                ETSOC_SHIRE_OTHER_ESR_SHIRE_DLL_READ_DATA_ADDRESS, 0) & 0x20000))
    {
      // Continue to poll till DLL is locked
      // Need to implement timeout mechanism
    } 
   return 0;
}

static int minion_configure_plls_and_dlls(uint64_t shire_mask)
{
    int status = MINION_PLL_DLL_CONFIG_ERROR;
    for (uint8_t i = 0; i <= 32; i++)
    {
        if (shire_mask & 1) {
          status = pll_config(i);
        }
        shire_mask >>= 1;
    }
   return status;
}

static int mm_get_error_count(struct mm_error_count_t *mm_error_count)
{
    mm_error_count->hang_count = event_control_block.hang_count;
    mm_error_count->exception_count = event_control_block.except_count;
    return 0;
}

static void minion_error_update_count(uint8_t error_type)
{
    struct event_message_t message;

    switch (error_type)
    {
        case EXCEPTION:
            if(++event_control_block.except_count > event_control_block.except_threshold) {

                /* add details in message header and fill payload */
                FILL_EVENT_HEADER(&message.header, MINION_EXCEPT_TH,
                                    sizeof(struct event_message_t))
                FILL_EVENT_PAYLOAD(&message.payload, WARNING, 1024, 1, 0)

                /* call the callback function and post message */
                event_control_block.event_cb(CORRECTABLE, &message);
            }
            break;

        case HANG:
            if(++event_control_block.hang_count > event_control_block.hang_threshold) {

                /* add details in message header and fill payload */
                FILL_EVENT_HEADER(&message.header, MINION_HANG_TH,
                                    sizeof(struct event_message_t))
                FILL_EVENT_PAYLOAD(&message.payload, WARNING, 1020, 1, 0)

                /* call the callback function and post message */
                event_control_block.event_cb(CORRECTABLE, &message);
            }
            break;

        default:
            break;
        }
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_Enable_Shire_Cache_and_Neighborhoods
*
*   DESCRIPTION
*
*       This function enables minion Shire caches and neighborhoods
*
*   INPUTS
*
*       shire_mask shires to be configured
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int Minion_Enable_Shire_Cache_and_Neighborhoods(uint64_t shire_mask)
{
    for (uint8_t i = 0; i <= 32; i++)
    {
        if (shire_mask & 1)
        {
            /* Set Shire ID, enable cache and all Neighborhoods */
            const uint64_t config = ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_SHIRE_ID_SET(i) |
                                    ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_CACHE_EN_SET(1) |
                                    ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_NEIGH_EN_SET(0xF);
            write_esr_new(PP_MACHINE, i, REGION_OTHER, 2,
                            ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_ADDRESS, config, 0);
        }
        shire_mask >>= 1;
    }
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_Enable_Master_Shire_Threads
*
*   DESCRIPTION
*
*       This function enables mastershire threads.
*
*   INPUTS
*
*       mm_id master minion shire id
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int Minion_Enable_Master_Shire_Threads(uint8_t mm_id)
{
    /* Enable only Device Runtime Management thread on Master Shire */
    write_esr_new(PP_MACHINE, mm_id, REGION_OTHER, 2, ETSOC_SHIRE_OTHER_ESR_THREAD0_DISABLE_ADDRESS,
                    ~(MM_RT_THREADS), 0);
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_Shire_Update_Voltage
*
*   DESCRIPTION
*
*       This function provide support to update the Minion
        Shire Power Rails.
*
*   INPUTS
*
*       voltage     value of the Voltage to updated to
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int Minion_Shire_Update_Voltage( uint8_t voltage)
{
    return pmic_set_voltage(MINION, voltage);
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_Get_Voltage_Given_Freq
*
*   DESCRIPTION
*
*       This function returns a voltage operating value given a freq value.
*
*   INPUTS
*
*       freq            Target Minion Frequency 
*
*   OUTPUTS
*
*       voltage          Target Minion Voltage 
*
***********************************************************************/
int Minion_Get_Voltage_Given_Freq(int32_t target_frequency)
{
    int target_voltage = THRESHOLD_VOLTAGE + (target_frequency * DeltaVolt_per_DeltaFreq);
     
    return target_voltage;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_Program_Step_Clock_PLL
*
*   DESCRIPTION
*
*       This function provide support to program the
        Minion Shire Step Clock which is coming from
        IO Shire HDPLL 4.
*
*   INPUTS
*
*       mode     value of the freq(in mode) to updated to
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int Minion_Program_Step_Clock_PLL(uint8_t mode)
{
    configure_sp_pll_4(mode);

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_Configure_Minion_Clock_Reset
*
*   DESCRIPTION
*
*       This function configures the Minion PLLs to Step Clock, and bring them
*       out of reset.
*
*   INPUTS
*
*       minion_shires_mask  Shire Mask to enable
*       Frequency mode to bring up Minions
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int Minion_Configure_Minion_Clock_Reset(uint64_t minion_shires_mask, uint8_t mode )
{

    /* TODO: Update Minion Voltage if neccesary
      Minion_Shire_Voltage_Update(voltage);
    */
    /* Configure Minon Step clock to 650 Mhz */
    if (0 != Minion_Program_Step_Clock_PLL(mode)) {
        Log_Write(LOG_LEVEL_ERROR, "configure_sp_pll_4() failed!\n");
        return MINION_STEP_CLOCK_CONFIGURE_ERROR;
    }

    if (0 != release_minions_from_cold_reset()) {
        Log_Write(LOG_LEVEL_ERROR, "release_minions_from_cold_reset() failed!\n");
        return MINION_COLD_RESET_CONFIG_ERROR;
    }

    if (0 != release_minions_from_warm_reset()) {
        Log_Write(LOG_LEVEL_ERROR, "release_minions_from_warm_reset() failed!\n");
        return MINION_WARM_RESET_CONFIG_ERROR ;
    }

    if (0 != minion_configure_plls_and_dlls(minion_shires_mask)) {
        Log_Write(LOG_LEVEL_ERROR, "minion_configure_plls_and_dlls() failed!\n");
        return MINION_PLL_DLL_CONFIG_ERROR;
    }

   return SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_Get_Active_Compute_Minion_Mask
*
*   DESCRIPTION
*
*       This function gets the active compute shire mask
        by reading the value from SP OTP.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       Active CM shire mask
*
***********************************************************************/
uint64_t Minion_Get_Active_Compute_Minion_Mask(void)
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

/************************************************************************
*
*   FUNCTION
*
*       Minion_Load_Authenticate_Firmware
*
*   DESCRIPTION
*
*       This function loads and authenticates the
        Minions firmware.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int Minion_Load_Authenticate_Firmware(void)
{
    if (0 != load_sw_certificates_chain()) {
        Log_Write(LOG_LEVEL_ERROR, "Failed to load SW ROOT/Issuing Certificate chain!\n");
        return FW_SW_CERTS_LOAD_ERROR;
    }

    if (0 != load_firmware(ESPERANTO_IMAGE_TYPE_MACHINE_MINION)) {
        Log_Write(LOG_LEVEL_ERROR, "Failed to load Machine Minion firmware!\n");
        return FW_MACH_LOAD_ERROR;
    }
    Log_Write(LOG_LEVEL_INFO, "MACH FW loaded.\n");

    if (0 != load_firmware(ESPERANTO_IMAGE_TYPE_MASTER_MINION)) {
        Log_Write(LOG_LEVEL_ERROR, "Failed to load Master Minion firmware!\n");
        return FW_MM_LOAD_ERROR;
    }
    Log_Write(LOG_LEVEL_INFO, "MM FW loaded.\n");

    if (0 != load_firmware(ESPERANTO_IMAGE_TYPE_WORKER_MINION)) {
        Log_Write(LOG_LEVEL_ERROR, "Failed to load Worker Minion firmware!\n");
        return FW_CM_LOAD_ERROR;
    }
    Log_Write(LOG_LEVEL_INFO, "WM FW loaded.\n");

   return SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_Shire_Update_PLL_Freq
*
*   DESCRIPTION
*
*       This function supports updating the Minion
        Shire PLL dynamically without stoppong the cores.
*
*   INPUTS
*
*       Frequency mode to bring up Minions
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int Minion_Shire_Update_PLL_Freq(uint8_t mode)
{
    struct sp2mm_update_freq_cmd_t cmd;

    cmd.msg_hdr.msg_id = SP2MM_CMD_UPDATE_FREQ;
    cmd.msg_hdr.msg_size = sizeof(struct sp2mm_update_freq_cmd_t);
    cmd.freq = mode;

    MM_Iface_Push_Cmd_To_SP2MM_SQ(&cmd, sizeof(cmd));

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_Read_ESR
*
*   DESCRIPTION
*
*       This function supports reading a Minion Shire
        ESR and returns value read.
*
*   INPUTS
*
*       ESR address offset
*
*   OUTPUTS
*
*       value on offset of register address
*
***********************************************************************/
uint64_t Minion_Read_ESR(uint32_t address)
{
    const volatile uint64_t *p = esr_address(PP_MACHINE, 0, REGION_MINION, address);
    return *p;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_Write_ESR
*
*   DESCRIPTION
*
*       This function supports writing a Minion Shire ESR.
*
*   INPUTS
*
*       ESR address offset
*       Data to be written to
*       Minion Shire Mask
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int Minion_Write_ESR(uint32_t address, uint64_t data, uint64_t mmshire_mask)
{

    for (uint8_t i = 0; i <= 33; i++) {
        if (mmshire_mask & 1) {
                volatile uint64_t *p = esr_address(PP_MACHINE, i, REGION_MINION, address);
                *p = data;
        }
        mmshire_mask >>= 1;
    }
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_Kernel_Launch
*
*   DESCRIPTION
*
*       This function supports launching a compute Kernel on specific
*       Shires.
*
*   INPUTS
*
*       Minion Shire mask to launch Compute kernel on
*       Arguments to the Compute Kernel
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int Minion_Kernel_Launch(uint64_t mmshire_mask, void *args)
{
    struct sp2mm_kernel_launch_cmd_t cmd;

    cmd.msg_hdr.msg_id = SP2MM_CMD_KERNEL_LAUNCH;
    cmd.msg_hdr.msg_size = sizeof(struct sp2mm_kernel_launch_cmd_t);
    cmd.args = args;
    cmd.shire_mask=mmshire_mask;

    MM_Iface_Push_Cmd_To_SP2MM_SQ(&cmd, sizeof(cmd));

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_Set_Active_Shire_Mask(minion_shires_mask)
*
*   DESCRIPTION
*
*       This function set the active Shire Mask global
*
*   INPUTS
*
*       uint64_t  Mask of active Shires
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Minion_Set_Active_Shire_Mask(uint64_t active_shire_mask)
{
    g_active_shire_mask = active_shire_mask;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_State_MM_Iface_Get_Active_Shire_Mask
*
*   DESCRIPTION
*
*       This function returns current active shires mask.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint64_t Current active shires mask.
*
***********************************************************************/
uint64_t Minion_State_MM_Iface_Get_Active_Shire_Mask(void)
{
    return g_active_shire_mask;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_State_Host_Iface_Process_Request
*
*   DESCRIPTION
*
*       This function process Host requests related to Master Minion state.
*
*   INPUTS
*
*       tag_id_t Unique tag ID of incoming command.
        msg_id   Message ID of incoming command.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Minion_State_Host_Iface_Process_Request(tag_id_t tag_id, msg_id_t msg_id)
{
    uint64_t req_start_time;
    int32_t status;
    req_start_time = timer_get_ticks_count();

    if (msg_id != DM_CMD_GET_MM_ERROR_COUNT)  {
        Log_Write(LOG_LEVEL_ERROR, " mm state svc error: Invalid Minion State Request id: %d\r\n",msg_id);
    } else {
        struct device_mgmt_mm_state_rsp_t dm_rsp;

        status = mm_get_error_count(&dm_rsp.mm_error_count);

        if (0 != status) {
             Log_Write(LOG_LEVEL_ERROR, " mm state svc error: get_mm_error_count()\r\n");
        } else {
            FILL_RSP_HEADER(dm_rsp, tag_id, DM_CMD_GET_MM_ERROR_COUNT,
                        timer_get_ticks_count() - req_start_time, status)

            if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_mm_state_rsp_t))) {
                  Log_Write(LOG_LEVEL_ERROR, "Minion_State_Host_Iface_Process_Request: Cqueue push error!\n");
            }
        } 
    }
}
/************************************************************************
*
*   FUNCTION
*
*       Minion_State_MM_Error_Handler
*
*   DESCRIPTION
*
*       This function process the Minion State errors.
*
*   INPUTS
*
*       int32_t Unique enum representing specific error.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Minion_State_MM_Error_Handler(int32_t error_code)
{
    switch (error_code) {
        case MM_HANG_ERROR: {
            /* update error count for Hang type */
            minion_error_update_count(HANG);
            break;
        }
        default:{
            /* update error count for Exception type */
            minion_error_update_count(EXCEPTION);
            break;
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_State_Error_Control_Init
*
*   DESCRIPTION
*
*       This function initializes the Minion error control subsystem, including
        programming the default error thresholds, enabling the error interrupts
        and setting up globals.
*
*   INPUTS
*
*       event_cb  callback to event.
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int32_t Minion_State_Error_Control_Init(dm_event_isr_callback event_cb)
{
    /* register event callback */
    event_control_block.event_cb = event_cb;

    /* Set default counters to zero. */
    event_control_block.except_count = 0;
    event_control_block.hang_count = 0;

    /* set default thershold values */
    event_control_block.except_threshold = 5;
    event_control_block.hang_threshold = 1;

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_State_Error_Control_Deinit
*
*   DESCRIPTION
*
*       This function de-initializes minion error control mechanism.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int32_t Minion_State_Error_Control_Deinit(void)
{
    event_control_block.event_cb = NULL;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_State_Set_Exception_Error_Threshold
*
*   DESCRIPTION
*
*       This function sets exception error threshold value.
*
*   INPUTS
*
*       th_value    threshold value.
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int32_t Minion_State_Set_Exception_Error_Threshold(uint32_t th_value)
{
    /* set errors count threshold */
    event_control_block.except_threshold = th_value;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_State_Set_Hang_Error_Threshold
*
*   DESCRIPTION
*
*       This function sets hang error threshold value.
*
*   INPUTS
*
*       th_value    threshold value.
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int32_t Minion_State_Set_Hang_Error_Threshold(uint32_t th_value)
{
    /* set errors count threshold */
    event_control_block.hang_threshold = th_value;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_State_Get_Exception_Error_Count
*
*   DESCRIPTION
*
*       This function return exception error count value.
*
*   INPUTS
*
*       th_value    threshold value.
*
*   OUTPUTS
*
*       err_count   error count value
*
***********************************************************************/
int32_t Minion_State_Get_Exception_Error_Count(uint32_t *err_count)
{
    /* get exceptionerrors count */
    *err_count = event_control_block.except_count;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_State_Get_Hang_Error_Count
*
*   DESCRIPTION
*
*       This function return hang error count value.
*
*   INPUTS
*
*       th_value    threshold value.
*
*   OUTPUTS
*
*       err_count   error count value
*
***********************************************************************/
int32_t Minion_State_Get_Hang_Error_Count(uint32_t *err_count)
{
    /* get hang errors count */
    *err_count = event_control_block.hang_count;
    return 0;
}
