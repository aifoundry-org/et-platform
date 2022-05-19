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
        Minion_Disable_CM_Shire_Threads
        Master_Minion_Reset
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
        Minion_State_MM_Heartbeat_Handler
        Minion_State_Get_MM_Heartbeat_Count
        Minion_State_MM_Error_Handlers
        Minion_State_Error_Control_Init
        Minion_State_Error_Control_Deinit
        Minion_State_Set_Exception_Error_Threshold
        Minion_State_Set_Hang_Error_Threshold
        Minion_State_Get_Exception_Error_Count
        Minion_State_Get_Hang_Error_Count
        Minion_VPU_RF_Init
*/
/***********************************************************************/
#include <stdio.h>
#include "delays.h"

#include <hwinc/etsoc_shire_other_esr.h>
#include <hwinc/etsoc_shire_cache_esr.h>
#include <hwinc/lvdpll_modes_config.h>

#include "esr.h"
#include "bl2_otp.h"
#include "minion_configuration.h"
#include "command_dispatcher.h"
#include "hal_minion_pll.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "bl2_otp.h"
#include "perf_mgmt.h"

#include "minion_state_inspection.h"
#include "minion_run_control.h"
#include "trace.h"
#include "dm_event_control.h"

/*!
 * @struct struct minion_event_control_block
 * @brief Minion error event mgmt control block
 */
struct minion_event_control_block
{
    uint64_t mm_heartbeat_count;    /**< counter to track MM heartbeat. */
    uint32_t except_count;          /**< Exception error count. */
    uint32_t hang_count;            /**< Hang error count. */
    uint32_t except_threshold;      /**< Exception error count threshold. */
    uint32_t hang_threshold;        /**< Hang error count threshold. */
    uint64_t active_shire_mask;     /**< active shire number mask. */
    dm_event_isr_callback event_cb; /**< Event callback handler. */
    bool mm_watchdog_initialized;   /**< Track state of mm watchdog. */
};

/* The driver can populate this structure with the defaults that will be used during the init
    phase.*/

static struct minion_event_control_block event_control_block __attribute__((section(".data")));

/* Globals for SW timer */
TimerHandle_t MM_HeartBeat_Timer;
static StaticTimer_t MM_Timer_Buffer;

/* Macro to increment exception error counter and send event to host */
#define MINION_EXCEPT_ERROR_EVENT_HANDLE(error_type, error_code)                             \
    /* Update error count */                                                                 \
    if (++event_control_block.except_count > event_control_block.except_threshold)           \
    {                                                                                        \
        struct event_message_t message;                                                      \
                                                                                             \
        /* Reset the errors counter */                                                       \
        event_control_block.except_count = 0;                                                \
                                                                                             \
        /* Add details in message header and fill payload */                                 \
        FILL_EVENT_HEADER(&message.header, MINION_EXCEPT_TH, sizeof(struct event_message_t)) \
        FILL_EVENT_PAYLOAD(&message.payload, WARNING, event_control_block.except_count,      \
                           error_type, (uint32_t)error_code)                                 \
                                                                                             \
        /* Call the callback function and post message */                                    \
        event_control_block.event_cb(CORRECTABLE, &message);                                 \
    }

/* Macro to update all Neighs in Shire for a given input function */
#define UPDATE_ALL_NEIGH(function, shireid)                             \
    for (uint8_t neighid = 0; neighid < NUM_NEIGH_PER_SHIRE; neighid++) \
    {                                                                   \
        function(shireid, neighid);                                     \
    }

/* Macro to update all Minion Harts in Shire for a given input function */
#define UPDATE_ALL_MINION(function, start_hart, last_hart)            \
    for (uint64_t hartid = start_hart; hartid <= last_hart; hartid++) \
    {                                                                 \
        function(hartid);                                             \
    }

/* Macro to update all Shires for a given input function */
#define UPDATE_ALL_SHIRE(shiremask, function, arg1, arg2, arg3) \
    for (uint8_t i = 0; i <= num_shires; i++)                   \
    {                                                           \
        if (shiremask & 1)                                      \
        {                                                       \
            function(i, arg1, arg2, arg3);                      \
        }                                                       \
        shiremask >>= 1;                                        \
    }

/* Macro to switch to Step Clock and disable LVDPLL     */
#define SWITCH_TO_STEP_CLOCK                              \
    (shiremask) for (uint8_t i = 0; i <= num_shires; i++) \
    {                                                     \
        if (shiremask & 1)                                \
        {                                                 \
            SWITCH_CLOCK_MUX(shireid, SELECT_STEP_CLOCK)  \
            lvdpll_disable(shireid);                      \
        }                                                 \
        shiremask >>= 1;                                  \
    }

#define CONFIG_SHIRE_NEIGH(id, sc_enable, neigh_mask, enable_vpu_rf_wa)                  \
    /* Set Shire ID, enable cache and all Neighborhoods */                               \
    const uint64_t config = ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_SHIRE_ID_SET(id) |        \
                            ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_CACHE_EN_SET(sc_enable) | \
                            ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_NEIGH_EN_SET(neigh_mask); \
    write_esr_new(PP_MACHINE, i, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER,                \
                  ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_ADDRESS, config, 0);                \
    if (enable_vpu_rf_wa)                                                                \
    {                                                                                    \
        /* VPU Array init */                                                             \
        if (0 != Minion_VPU_RF_Init(i))                                                  \
            Log_Write(LOG_LEVEL_WARNING, "Shire %d VPU RF not initialized\n", i);        \
    }

/*! \def FREQUENCY_HZ_TO_MHZ(x)
    \brief Converts HZ to MHZ
*/
#define FREQUENCY_HZ_TO_MHZ(x) ((x) / 1000000u)

/* This Threashold voltage would need to be extracted from OTP */
#define THRESHOLD_VOLTAGE 400

/* This chracterized co-efficient needs to be extracted from OTP */
#define DeltaVolt_per_DeltaFreq 0

// Define for Threads which participate in the Device Runtime FW management.
// Currently only lower 16 Minions (64 Harts) of the whole Minion Shire which
// participate in the Device Runtime. This might change with Virtual Queue
// implementation
#define MM_RT_THREADS 0x0000FFFFU

/*! \def MM_COMPUTE_THREADS
    \brief Define for Threads which participate in the Device Runtime.
         Currently the upper 16 Minions (32 Harts) of the whole Minion Shire
         participates in Compute Minion Kernel execution.
*/
#define MM_COMPUTE_THREADS 0xFFFFFFFFU

static uint64_t gs_active_shire_mask = 0;
static uint64_t gs_dlls_initialized = 0;

/* MM command handler task cb to reset task */
extern TaskHandle_t g_mm_cmd_hdlr_handle;

/* MM heartbeat time CB to delete timer */
extern TimerHandle_t MM_HeartBeat_Timer;

/*==================== Function Separator =============================*/

/*! \def TIMEOUT_PLL_CONFIG
    \brief PLL timeout configuration
*/
#define TIMEOUT_PLL_CONFIG 100000

/*! \def TIMEOUT_PLL_LOCK
    \brief lock timeout for PLL
*/
#define TIMEOUT_PLL_LOCK 100000

/*==================== Function Separator =============================*/

/************************************************************************
*
*   FUNCTION
*
*       get_highest_set_bit_offset
*
*   DESCRIPTION
*
*       This function returns highest set bit offset
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
static uint8_t get_highest_set_bit_offset(uint64_t shire_mask)
{
    return (uint8_t)(64 - __builtin_clzl(shire_mask));
}

/************************************************************************
*
*   FUNCTION
*
*       clock_gate_debug_logic
*
*   DESCRIPTION
*
*       This function clock gates the Minion Shire Debug logic
*
*   INPUTS
*
*       N/A
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/

#define CLOCK_GATE_DEBUG_LOGIC                                                         \
    for (uint8_t id = 0; id <= 33; id++)                                               \
    {                                                                                  \
        write_esr_new(PP_MACHINE, id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER,         \
                      ETSOC_SHIRE_OTHER_ESR_DEBUG_CLK_GATE_CTRL_BYTE_ADDRESS, 0x1, 0); \
    }

/************************************************************************
*
*   FUNCTION
*
*       minion_configure_plls_and_dlls
*
*   DESCRIPTION
*
*       This function configures minion shires DLL and PLL
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
static int minion_configure_plls_and_dlls(uint64_t shire_mask, uint8_t mode)
{
    int status = SUCCESS;
    uint64_t pll_fail_mask = 0;
    uint8_t num_shires;

    if (0 != shire_mask)
    {
        num_shires = get_highest_set_bit_offset(shire_mask);
    }
    else
    {
        return MINION_INVALID_SHIRE_MASK;
    }
    for (uint8_t i = 0; i <= num_shires; i++)
    {
        if (shire_mask & 1)
        {
            SWITCH_CLOCK_MUX(i, SELECT_STEP_CLOCK)

            if (0 != config_lvdpll_freq_full(i, mode))
            {
                status = MINION_PLL_CONFIG_ERROR;
                pll_fail_mask = pll_fail_mask | (uint64_t)(1 << i);
            }
            else
            {
                SWITCH_CLOCK_MUX(i, SELECT_PLL_CLOCK_0)
            }

            lvdpll_clear_lock_monitor(i);
        }
        shire_mask >>= 1;
    }
    if (status != SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "minion_configure_plls_and_dlls(): PLL failed mask %lu!\n",
                  pll_fail_mask);
    }
    else
    {
        Update_Minion_Frequency_Global_Reg(
            (int32_t)FREQUENCY_HZ_TO_MHZ(gs_lvdpll_settings[mode - 1].output_frequency));
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       print_minion_shire_lvdpll_lock_monitors
*
*   DESCRIPTION
*
*       This function prints lock monitors of LVDPLLs
*
*   INPUTS
*
*       shire_mask shires for which lock monitor will be printed
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int print_minion_shire_lvdpll_lock_monitors(uint64_t shire_mask)
{
    uint8_t num_shires;
    uint16_t lock_monitor;

    if (0 != shire_mask)
    {
        num_shires = get_highest_set_bit_offset(shire_mask);
    }
    else
    {
        return MINION_INVALID_SHIRE_MASK;
    }

    for (uint8_t i = 0; i <= num_shires; i++)
    {
        if (shire_mask & 1)
        {
            lvdpll_read_lock_monitor(i, &lock_monitor);
            if (0 != lock_monitor)
            {
                Log_Write(LOG_LEVEL_WARNING, "MINSHIRE %d PLL lock monitor: %d\n", i,
                          lock_monitor & 0x3F);
            }
        }
        shire_mask >>= 1;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       clear_minion_shire_lvdpll_lock_monitors
*
*   DESCRIPTION
*
*       This function clears lock monitors of LVDPLLs
*
*   INPUTS
*
*       shire_mask shires for which lock monitor will be cleared
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int clear_minion_shire_lvdpll_lock_monitors(uint64_t shire_mask)
{
    uint8_t num_shires;

    if (0 != shire_mask)
    {
        num_shires = get_highest_set_bit_offset(shire_mask);
    }
    else
    {
        return MINION_INVALID_SHIRE_MASK;
    }

    for (uint8_t i = 0; i <= num_shires; i++)
    {
        if (shire_mask & 1)
        {
            lvdpll_clear_lock_monitor(i);
        }
        shire_mask >>= 1;
    }

    return 0;
}

static int mm_get_error_count(struct mm_error_count_t *mm_error_count)
{
    mm_error_count->hang_count = event_control_block.hang_count;
    mm_error_count->exception_count = event_control_block.except_count;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       MM_HeartBeat_Timer_Cb
*
*   DESCRIPTION
*
*       Timer callback function
*
*   INPUTS
*
*       pxTimer     Timer handle
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void MM_HeartBeat_Timer_Cb(xTimerHandle pxTimer)
{
    Log_Write(LOG_LEVEL_ERROR, "%s : MM heartbeat watchdog timer expired\n", __func__);

    Minion_State_MM_Error_Handler(SP_RECOVERABLE_FW_MM_HANG, MM_HEARTBEAT_WD_EXPIRED);

    /* Stop the timer */
    if (pdPASS != xTimerStop(pxTimer, 0))
    {
        Log_Write(LOG_LEVEL_ERROR, "%s : MM heartbeat watchdog timer stop failed\n", __func__);
    }

    /* Reset minion threads */
    Master_Minion_Reset();
}

/************************************************************************
*
*   FUNCTION
*
*       enable_minion_shire
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
static int enable_minion_shire(uint64_t shire_mask)
{
    uint8_t num_shires;
    if (0 != shire_mask)
    {
        num_shires = get_highest_set_bit_offset(shire_mask);
    }
    else
    {
        return MINION_INVALID_SHIRE_MASK;
    }

    uint64_t shiremask = shire_mask;

#if !(FAST_BOOT || TEST_FRAMEWORK)
    /* Enable Minion in all neighs */
    UPDATE_ALL_SHIRE(shiremask, CONFIG_SHIRE_NEIGH, 1 /* Enable S$*/, 0xF /*Enable all Neigh*/,
                     true /*Enable VPU RF WA*/)

    /* Due to workaround to initialize Minion VPU RF using Minion Program Buffer 
       we need to reset the Neigh/Minion Core to bring it back to clean state */

    /* Disable Minion in all neighs */
    shiremask = shire_mask;
    UPDATE_ALL_SHIRE(shiremask, CONFIG_SHIRE_NEIGH, 0 /* Disable S$*/, 0x0 /*Disable all Neigh*/,
                     false)

    /* Enable Minion in all neighs */
    shiremask = shire_mask;
    UPDATE_ALL_SHIRE(shiremask, CONFIG_SHIRE_NEIGH, 1 /* Enable S$*/, 0xF /*Enable all Neigh*/,
                     false)

    /* Clock gate debug logic */
    CLOCK_GATE_DEBUG_LOGIC

    Log_Write(LOG_LEVEL_CRITICAL, "Shire Cache and Neigh Enable with VPU RF WA\n");
#else
    /* Enable Minion in all neighs */
    UPDATE_ALL_SHIRE(shiremask, CONFIG_SHIRE_NEIGH, 1 /* Enable S$*/, 0xF /*Enable all Neigh*/,
                     false)
#endif

    return SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       minion_configure_hpdpll
*
*   DESCRIPTION
*
*       This function configures the Minion Shire with HPDPLL mode
*
*   INPUTS
*
*       hpdpll_mode - value of the Step Clock freq(in mode)
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
static int minion_configure_hpdpll(uint8_t hpdpll_mode, uint64_t shire_mask)
{
    uint32_t freq;
    int status;

    status = configure_sp_pll_4(hpdpll_mode);
    if (status != SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "configure_sp_pll_4() failed!\n");
        return MINION_STEP_CLOCK_CONFIGURE_ERROR;
    }

    status = get_pll_frequency(PLL_ID_SP_PLL_4, &freq);
    if (status != SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "get_pll_frequency(): failed to get frequency!\n");
        return MINION_GET_FREQ_ERROR;
    }

    uint8_t num_shires;
    if (0 != shire_mask)
    {
        num_shires = get_highest_set_bit_offset(shire_mask);
    }
    else
    {
        return MINION_INVALID_SHIRE_MASK;
    }

    for (uint8_t shireid = 0; shireid <= num_shires; shireid++)
    {
        if (shire_mask & 1)
        {
            SWITCH_CLOCK_MUX(shireid, SELECT_STEP_CLOCK)
            lvdpll_disable(shireid);
        }
        shire_mask >>= 1;
    }

    Update_Minion_Frequency_Global_Reg((int32_t)freq);

    return SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_Enable_Master_Shire_Threads
*
*   DESCRIPTION
*
*       This function configures mastershire threads.
*
*   INPUTS
*
*       enable flag to enable/disable MM threads
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int Minion_Enable_Master_Shire_Threads(void)
{
    uint8_t mm_id = 0;
    int status;
    uint32_t yield_priority = 0x1;
    uint64_t sc_esr;

    // Extract Master Minion Shire ID from Fuses
    status = otp_get_master_shire_id(&mm_id);
    if (status != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "Failed to read master minion shire ID");
    }

    // If ID Value wasn't burned into fuse, use default value
    if (mm_id == 0xff)
    {
        Log_Write(LOG_LEVEL_WARNING,
                  "Master shire ID was not found in the OTP, using default value of 32!\n");
        mm_id = 32;
    }

    // Set SC REQQ to distribute arbritration equally between Neigh/L3 Slave
    for (uint8_t bank = 0; bank < 4; bank++)
    {
        sc_esr = read_esr_new(PP_MACHINE, mm_id, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                              ETSOC_SHIRE_CACHE_ESR_SC_REQQ_CTL_ADDRESS, bank);
        sc_esr = ETSOC_SHIRE_CACHE_ESR_SC_REQQ_CTL_ESR_SC_L3_YIELD_PRIORITY_MODIFY(sc_esr,
                                                                                   yield_priority);
        write_esr_new(PP_MACHINE, mm_id, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                      ETSOC_SHIRE_CACHE_ESR_SC_REQQ_CTL_ADDRESS, sc_esr, bank);
    }

    /* Enable only Device Runtime threads on Master Shire */
    write_esr_new(PP_MACHINE, mm_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER,
                  ETSOC_SHIRE_OTHER_ESR_THREAD0_DISABLE_ADDRESS, ~(MM_RT_THREADS), 0);

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_Configure_CM_Shire_Threads
*
*   DESCRIPTION
*
*       This function configures CM threads.
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
int Minion_Disable_CM_Shire_Threads(void)
{
    uint64_t shire_mask = Minion_State_MM_Iface_Get_Active_Shire_Mask();
    uint8_t num_shires = get_highest_set_bit_offset(shire_mask);

    for (uint8_t i = 0; i <= num_shires; i++)
    {
        if (shire_mask & 1)
        {
            /* Disable all CM threads on each Shire */
            write_esr_new(PP_MACHINE, i, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER,
                          ETSOC_SHIRE_OTHER_ESR_THREAD0_DISABLE_ADDRESS, MM_COMPUTE_THREADS, 0);
            write_esr_new(PP_MACHINE, i, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER,
                          ETSOC_SHIRE_OTHER_ESR_THREAD1_DISABLE_ADDRESS, MM_COMPUTE_THREADS, 0);
        }
        shire_mask >>= 1;
    }

    return 0;
}
/************************************************************************
*
*   FUNCTION
*
*       Master_Minion_Reset
*
*   DESCRIPTION
*
*       This function resets the Master Minion Shire threads.
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
int Master_Minion_Reset(void)
{
    uint64_t shire_mask = Minion_State_MM_Iface_Get_Active_Shire_Mask();
    uint8_t num_shires = get_highest_set_bit_offset(shire_mask);
    int status = MM_NOT_READY;
    uint64_t time_end = 0;

    /* Disable Minion neighs */
    for (uint8_t i = 0; i <= num_shires; i++)
    {
        if (shire_mask & 1)
        {
            /* Set values for Shire ID, enable cache and all Neighborhoods */
            const uint64_t config = ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_SHIRE_ID_SET(i) |
                                    ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_CACHE_EN_SET(0) |
                                    ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_NEIGH_EN_SET(0x0);
            write_esr_new(PP_MACHINE, i, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER,
                          ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_ADDRESS, config, 0);
        }
        shire_mask >>= 1;
    }

    /* Disable CM threads  */
    Minion_Disable_CM_Shire_Threads();

    /* Delete MM command handler task to re initialize it*/
    vTaskDelete(g_mm_cmd_hdlr_handle);

    /* Delete MM heartbeat timer before disabling MM threads */
    xTimerDelete(MM_HeartBeat_Timer, 0);

    /* Get active shire mask */
    shire_mask = Minion_State_MM_Iface_Get_Active_Shire_Mask();

    /* Enab Minion neighs. Minion threads will be enabled by MM when it will bring up CMs after reset */
    for (uint8_t i = 0; i <= num_shires; i++)
    {
        if (shire_mask & 1)
        {
            /* Set Shire ID, enable cache and all Neighborhoods */
            const uint64_t config = ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_SHIRE_ID_SET(i) |
                                    ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_CACHE_EN_SET(1) |
                                    ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_NEIGH_EN_SET(0xf);
            write_esr_new(PP_MACHINE, i, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER,
                          ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_ADDRESS, config, 0);
        }
        shire_mask >>= 1;
    }

    /* Re initialize SP-MM services */
    if (SP_MM_Iface_Init() != 0)
    {
        Log_Write(LOG_LEVEL_ERROR, "MM_Iface_Init Failed\n");
    }
    else
    {
        /* Re launch MM command handler */
        launch_mm_sp_command_handler();

        if (Minion_State_Error_Control_Init(minion_event_callback) != 0)
        {
            Log_Write(LOG_LEVEL_ERROR, "Minion_State_Error_Control_Init Failed\n");
        }
        /* Initialize heart beat timer */
        if (MM_Init_HeartBeat_Watchdog() != 0)
        {
            Log_Write(LOG_LEVEL_ERROR, "MM_Init_HeartBeat_Watchdog Failed\n");
        }
        else
        {
            /* Bringing MM RT and CM harts out of reset */
            Minion_Enable_Master_Shire_Threads();

            /*TODO: Will be removed after SW-11279 */
            Trace_Init_SP(NULL);
        }
    }

    /* wait for MM ready flag */
    time_end = timer_get_ticks_count() + pdMS_TO_TICKS(MM_RESET_TIMEOUT_MSEC);
    while (timer_get_ticks_count() < time_end)
    {
        /* Wait to receive Heartbeat from MM */
        if (Minion_State_Get_MM_Heartbeat_Count() > 0)
        {
            status = MM_READY;
            break;
        }
        vTaskDelay(1);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       Compute_Minion_Reset_Threads
*
*   DESCRIPTION
*
*       This function resets the Compute Minion Shire threads.
*
*   INPUTS
*
*       minion_shires_mask Minion Shire Mask
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int Compute_Minion_Reset_Threads(uint64_t shires_mask)
{
    int32_t status = MINION_INVALID_SHIRE_MASK;

    if (0 == shires_mask)
    {
        return status;
    }

    /* Send CM reset command to MM */
    status = MM_Iface_Wait_For_CM_Boot_Cmd(shires_mask);

    return status;
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
int Minion_Shire_Update_Voltage(uint8_t voltage)
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
*       Minion_Configure_Minion_Shire_PLL
*
*   DESCRIPTION
*
*       This function configures the Minion Shire PLLs
*
*   INPUTS
*
*       hpdpll_mode - value of the Step Clock freq(in mode) to updated to
*       lvdpll_mode - value of Minion LVDPLLL freq(in mode)
*       use_step_clock - Option to have Minion Shire use Step Clock
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int Minion_Configure_Minion_Shire_PLL(uint64_t minion_shires_mask, uint8_t hpdpll_mode,
                                      uint8_t lvdpll_mode, bool use_step_clock)
{
    int status = SUCCESS;

    uint8_t num_shires;
    uint64_t shire_mask = minion_shires_mask;
    uint64_t dll_fail_mask = 0;

    if (0 != minion_shires_mask)
    {
        num_shires = get_highest_set_bit_offset(minion_shires_mask);
    }
    else
    {
        return MINION_INVALID_SHIRE_MASK;
    }

    for (uint8_t i = 0; i <= num_shires; i++)
    {
        if ((shire_mask & 1) && (0 == gs_dlls_initialized))
        {
            SWITCH_CLOCK_MUX(i, SELECT_REF_CLOCK)
            if (0 != dll_config(i))
            {
                dll_fail_mask = dll_fail_mask | (uint64_t)(1 << i);
            }
        }
        shire_mask >>= 1;
    }

    if (0 != dll_fail_mask)
    {
        Log_Write(LOG_LEVEL_ERROR, "minion_configure_dlls(): DLL failed mask %lu!\n",
                  dll_fail_mask);
        return MINION_DLL_CONFIG_ERROR;
    }
    else
    {
        gs_dlls_initialized = 1;
    }

    if (use_step_clock)
    {
        Log_Write(LOG_LEVEL_CRITICAL, "Minion Shire PLL using Step Clock\n");
        status = minion_configure_hpdpll(hpdpll_mode, minion_shires_mask);
    }
    else
    {
        Log_Write(LOG_LEVEL_CRITICAL, "Minion Shire PLL using LVDPLL Clock\n");
        status = minion_configure_plls_and_dlls(minion_shires_mask, lvdpll_mode);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_Configure_Minion_Shire_PLL_no_mask
*
*   DESCRIPTION
*
*       This function configures the Minion Shire PLLs
*
*   INPUTS
*
*       hpdpll_mode - value of the Step Clock freq(in mode) to updated to
*       lvdpll_mode - value of Minion LVDPLLL freq(in mode)
*       use_step_clock - Option to have Minion Shire use Step Clock
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int Minion_Configure_Minion_Shire_PLL_no_mask(uint8_t hpdpll_mode, uint8_t lvdpll_mode,
                                              bool use_step_clock)
{
    int status = SUCCESS;

    status = Minion_Configure_Minion_Shire_PLL(gs_active_shire_mask, hpdpll_mode, lvdpll_mode,
                                               use_step_clock);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*      Initialize_Minions
*
*   DESCRIPTION
*
*       This function brings all Minion shire out of reset and enables
*       all Shire Cache/Neigh logic, and clears up VPU state
*
*
*   INPUTS
*
*       shire_mask - Shire MASK
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int Initialize_Minions(uint64_t shire_mask)
{
    /* Update Minion Voltage if neccesary
       NOSONAR Minion_Shire_Voltage_Update(new_volt);*/

    if (0 != release_minions_from_cold_reset())
    {
        Log_Write(LOG_LEVEL_ERROR, "release_minions_from_cold_reset() failed!\n");
        return MINION_COLD_RESET_CONFIG_ERROR;
    }

    if (0 != release_minions_from_warm_reset())
    {
        Log_Write(LOG_LEVEL_ERROR, "release_minions_from_warm_reset() failed!\n");
        return MINION_WARM_RESET_CONFIG_ERROR;
    }

    if (0 != enable_minion_shire(shire_mask))
    {
        Log_Write(LOG_LEVEL_ERROR, "minions_enable_shire() failed!\n");
        return MINION_ENABLE_SHIRE_ERROR;
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
    for (uint32_t entry = 0; entry < 4; entry++)
    {
        uint32_t status;

        ret = sp_otp_get_neighborhood_status_mask(entry, &status);
        if (ret < 0)
        {
            // If the OTP read fails, assume we have to enable all Neighboorhods
            status = 0xFFFFFFFF;
        }

        // Each Neighboorhod status OTP entry contains information for 8 Shires
        for (uint32_t i = 0; i < 8; i++)
        {
            // Only enable a Shire if *ALL* its Neighboorhods are Functional
            if ((status & 0xF) == 0xF)
            {
                enable_mask |= 1ULL << (entry * 8 + i);
            }
            status >>= 4;
        }
    }

    // Master Shire Neighboorhods status
    ret = sp_otp_get_neighborhood_status_nh128_nh135_other(&status_other);
    if ((ret < 0) || ((status_other.B.neighborhood_status & 0xF) == 0xF))
    {
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
    if (0 != load_sw_certificates_chain())
    {
        Log_Write(LOG_LEVEL_ERROR, "Failed to load SW ROOT/Issuing Certificate chain!\n");
        return FW_SW_CERTS_LOAD_ERROR;
    }

    if (0 != load_firmware(ESPERANTO_IMAGE_TYPE_MACHINE_MINION))
    {
        Log_Write(LOG_LEVEL_ERROR, "Failed to load Machine Minion firmware!\n");
        return FW_MACH_LOAD_ERROR;
    }
    Log_Write(LOG_LEVEL_INFO, "MACH FW loaded.\n");

    if (0 != load_firmware(ESPERANTO_IMAGE_TYPE_MASTER_MINION))
    {
        Log_Write(LOG_LEVEL_ERROR, "Failed to load Master Minion firmware!\n");
        return FW_MM_LOAD_ERROR;
    }
    Log_Write(LOG_LEVEL_INFO, "MM FW loaded.\n");

    if (0 != load_firmware(ESPERANTO_IMAGE_TYPE_WORKER_MINION))
    {
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
*       Frequency to bring up Minions
*
*   OUTPUTS
*
*       The function call status, pass/fail
*
***********************************************************************/
int Minion_Shire_Update_PLL_Freq(uint16_t freq)
{
    struct sp2mm_update_freq_cmd_t cmd;

    cmd.msg_hdr.msg_id = SP2MM_CMD_UPDATE_FREQ;
    cmd.msg_hdr.msg_size = sizeof(struct sp2mm_update_freq_cmd_t);
    cmd.freq = freq;

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
    uint8_t num_shires;
    if (0 != mmshire_mask)
    {
        num_shires = get_highest_set_bit_offset(mmshire_mask);
    }
    else
    {
        return MINION_INVALID_SHIRE_MASK;
    }
    for (uint8_t i = 0; i <= num_shires; i++)
    {
        if (mmshire_mask & 1)
        {
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
    cmd.shire_mask = mmshire_mask;

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
    gs_active_shire_mask = active_shire_mask;
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
    return gs_active_shire_mask;
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
    struct device_mgmt_mm_state_rsp_t dm_rsp;

    req_start_time = timer_get_ticks_count();

    switch (msg_id)
    {
        case DM_CMD_MM_RESET:
            status = Master_Minion_Reset();
            if (status != SUCCESS)
            {
                Log_Write(LOG_LEVEL_ERROR, " mm state svc error: Master_Minion_Reset()\r\n");
            }
            break;
        case DM_CMD_GET_MM_ERROR_COUNT:
            status = mm_get_error_count(&dm_rsp.mm_error_count);
            if (0 != status)
            {
                Log_Write(LOG_LEVEL_ERROR, " mm state svc error: get_mm_error_count()\r\n");
            }
            break;
        default:
            Log_Write(LOG_LEVEL_ERROR,
                      " mm state svc error: Invalid Minion State Request id: %d\r\n", msg_id);
            status = -1;
    }

    FILL_RSP_HEADER(dm_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time, status)

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_mm_state_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "Minion_State_Host_Iface_Process_Request: Cqueue push error!\n");
    }

    return;
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
*       uint16_t Type of error occured.
*       int32_t Unique enum representing specific error.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Minion_State_MM_Error_Handler(uint16_t error_type, int32_t error_code)
{
    /* Check for MM hang event */
    if (error_type == SP_RECOVERABLE_FW_MM_HANG)
    {
        /* Update error count for Hang type */
        if (++event_control_block.hang_count > event_control_block.hang_threshold)
        {
            struct event_message_t message;

            /* Reset the hang counter */
            event_control_block.hang_count = 0;

            /* Add details in message header and fill payload */
            FILL_EVENT_HEADER(&message.header, MINION_HANG_TH, sizeof(struct event_message_t))
            FILL_EVENT_PAYLOAD(&message.payload, WARNING, event_control_block.hang_count,
                               SP_RECOVERABLE_FW_MM_HANG, (uint32_t)error_code)

            /* Call the callback function and post message */
            event_control_block.event_cb(CORRECTABLE, &message);
        }
    }
    else
    {
        /* Generate error event for the host */
        MINION_EXCEPT_ERROR_EVENT_HANDLE(error_type, error_code)
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_State_MM_Heartbeat_Handler
*
*   DESCRIPTION
*
*       Increment MM heartbeat
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void Minion_State_MM_Heartbeat_Handler(void)
{
    /* First time we get a heartbeat, we register watchdog timer */
    if (!event_control_block.mm_watchdog_initialized)
    {
        event_control_block.mm_watchdog_initialized = true;
        if (xTimerStart(MM_HeartBeat_Timer, 0) != pdPASS)
        {
            /* The start command was not executed successfully - Log Error */
            Log_Write(LOG_LEVEL_ERROR, "%s : MM heartbeat watchdog timer start failed\n", __func__);
        }
    }
    else
    {
        if (xTimerReset(MM_HeartBeat_Timer, 0) != pdPASS)
        {
            /* The reset command was not executed successfully - Log Error */
            Log_Write(LOG_LEVEL_ERROR, "%s : MM heartbeat watchdog timer reset failed\n", __func__);
        }
    }

    /* Increment mm heartbeat counter */
    event_control_block.mm_heartbeat_count++;
}

/************************************************************************
*
*   FUNCTION
*
*       Minion_State_Get_MM_Heartbeat_Count
*
*   DESCRIPTION
*
*       Get MM heartbeat
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       uint64_t    MM heartbeat count
*
***********************************************************************/
uint64_t Minion_State_Get_MM_Heartbeat_Count(void)
{
    return event_control_block.mm_heartbeat_count;
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
*       programming the default error thresholds, enabling the error interrupts
*       and setting up globals.
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

    /* Zeroize MM heartbeat count */
    event_control_block.mm_heartbeat_count = 0;
    event_control_block.mm_watchdog_initialized = false;

    /* Set default counters to zero. */
    event_control_block.except_count = 0;
    event_control_block.hang_count = 0;

    /* Set default threshold values */
    event_control_block.except_threshold = MINION_EXCEPT_ERROR_THRESHOLD;
    event_control_block.hang_threshold = MINION_HANG_ERROR_THRESHOLD;

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

/************************************************************************
*
*   FUNCTION
*
*       MM_Init_HeartBeat_Watchdog
*
*   DESCRIPTION
*
*      This function creates watchdog timer for MM  heartbeat.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       status   Status indicating success or negative error
*
***********************************************************************/
int8_t MM_Init_HeartBeat_Watchdog(void)
{
    int8_t status = 0;

    MM_HeartBeat_Timer = xTimerCreateStatic("MM_HEARTBEAT",
                                            pdMS_TO_TICKS(MM_HEARTBEAT_TIMEOUT_MSEC), pdFALSE,
                                            (void *)0, MM_HeartBeat_Timer_Cb, &MM_Timer_Buffer);
    if (!MM_HeartBeat_Timer)
    {
        Log_Write(LOG_LEVEL_ERROR, "%s : MM heartbeat watchdog timer creation failed\n", __func__);
        /* Todo: Need to change error types to int32_t */
        status = -1;
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*      enable_sram_and_icache_interrupts
*
*   DESCRIPTION
*
*      This function enables interrupts from SRAM and ICache.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       status   Status indicating success or negative error
*
***********************************************************************/
int8_t enable_sram_and_icache_interrupts(void)
{
    int8_t status = 0;
    uint8_t minshire;
    uint64_t shire_mask;

    shire_mask = Minion_Get_Active_Compute_Minion_Mask();

    FOR_EACH_MINSHIRE(
        INT_enableInterrupt(SPIO_PLIC_MINSHIRE_ERR0_INTR + minshire, 1, sram_and_icache_error_isr);)

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*      disable_sram_and_icache_interrupts
*
*   DESCRIPTION
*
*      This function disables interrupts from SRAM and ICache.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       status   Status indicating success or negative error
*
***********************************************************************/
int8_t disable_sram_and_icache_interrupts(void)
{
    int8_t status = 0;
    uint8_t minshire;
    uint64_t shire_mask;

    shire_mask = Minion_Get_Active_Compute_Minion_Mask();

    FOR_EACH_MINSHIRE(INT_disableInterrupt(SPIO_PLIC_MINSHIRE_ERR0_INTR + minshire);)

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*      Minion_VPU_RF_Init
*
*   DESCRIPTION
*
*      This function clears up all Minions Shires Core VPU RF.
*
*   INPUTS
*
*       shireid - Minion Shire ID to initialize
*
*   OUTPUTS
*
*       status   Status indicating success or negative error
*
************************************************************************/
int Minion_VPU_RF_Init(uint8_t shireid)
{
    /*Select all Neighs in Shire*/
    UPDATE_ALL_NEIGH(Select_Harts, shireid)

    /* Assert Halt */
    assert_halt();

    /* Enable all Threads in this Shire */
    enable_shire_threads(shireid);

    /* Wait for all Harts in Shire to halt */
    /* NOSONAR: if (!WAIT(check_halted())) {
       NOSONAR  UPDATE_ALL_NEIGH(Unselect_Harts,shireid)
       NOSONAR  return MINION_CORE_NOT_HALTED;
    }*/

    uint64_t start_hart = (uint64_t)shireid * HARTS_PER_SHIRE;
    uint64_t last_hart = start_hart + (HARTS_PER_SHIRE - 1);
    UPDATE_ALL_MINION(VPU_RF_Init, start_hart, last_hart)
    Log_Write(LOG_LEVEL_DEBUG, "Executed VPU RF on Hart [%ld:%ld]\n", start_hart, last_hart);

    /* Unselect all Neighs in Shire */
    UPDATE_ALL_NEIGH(Unselect_Harts, shireid)

    /* Disable all Threads in this Shire */
    disable_shire_threads(shireid);

    deassert_halt();

    return SUCCESS;
}