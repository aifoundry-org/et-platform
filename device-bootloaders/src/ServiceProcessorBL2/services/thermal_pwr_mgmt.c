/*************************************************************************
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
*************************************************************************/
/*! \file thermal_pwr_mgmt.c
    \brief A C module that abstracts the Power Management services

    Public interfaces:
    TODO

*/
/***********************************************************************/
#include "thermal_pwr_mgmt.h"
#include "perf_mgmt.h"
#include "bl2_pmic_controller.h"
#include "minion_configuration.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bl2_main.h"
#include "bl_error_code.h"

struct soc_power_reg_t g_soc_power_reg __attribute__((section(".data")));

#define TT_TASK_STACK_SIZE 1024
#define TT_TASK_PRIORITY   1

/* The variable used to hold task's handle */
static TaskHandle_t t_handle;

/* Task entry function */
static void thermal_power_task_entry(void *pvParameters);

/* Task stack memory */
static StackType_t g_pm_task[TT_TASK_STACK_SIZE];

/* The variable used to hold the task's data structure. */
static StaticTask_t g_staticTask_ptr;

/* The PMIC isr callback function, called from PMIC isr. */
static void pmic_isr_callback(enum error_type type, struct event_message_t *msg);

/* The variable used to track power states change time. */
static uint64_t power_state_change_time = 0;

struct soc_power_reg_t
{
    struct residency_t power_up_throttled_states_residency;
    struct residency_t power_down_throttled_states_residency;
    struct residency_t thermal_down_throttled_states_residency;
    struct residency_t power_safe_throttled_states_residency;
    struct residency_t thermal_safe_throttled_states_residency;
    power_state_e module_power_state;
    uint8_t module_tdp_level;
    uint8_t soc_temperature;
    uint8_t soc_power;
    uint8_t power_scale_factor;
    uint8_t max_temp;
    struct temperature_threshold_t temperature_threshold;
    struct module_uptime_t module_uptime;
    struct module_voltage_t module_voltage;
    struct residency_t power_max_residency;
    struct residency_t power_managed_residency;
    struct residency_t power_safe_residency;
    struct residency_t power_low_residency;
    dm_event_isr_callback event_cb;
    power_throttle_state_e power_throttle_state;
};

volatile struct soc_power_reg_t *get_soc_power_reg(void)
{
    return &g_soc_power_reg;
}

// TODO: Need to create mapping between Power State to actual value in W
#define POWER_STATE_TO_W(state) 25

// TODO: This needs to extracted from OTP Fuse
#define DP_per_Mhz 1

/*! \def FILL_POWER_STATUS(ptr, throttle_st, pwr_st, curr_pwr, curr_temp) 
    \brief Help macro to fill up Power Status Struct
*/
#define FILL_POWER_STATUS(ptr, u, v, w, x, y, z)       \
        ptr.throttle_state = u; ptr.power_state = v ;  \
        ptr.current_power = w ; ptr.current_temp = x;  \
        ptr.tgt_freq = y ; ptr.tgt_voltage = z;        \

/*! \def SEND_THROTTLE_EVENT(THROTTLE_EVENT_TYPE)
    \brief Sends throttle event to host
*/
#define SEND_THROTTLE_EVENT(THROTTLE_EVENT_TYPE)                                                   \
    if (get_soc_power_reg()->event_cb)                                                             \
    {                                                                                              \
        /* Send the event to the device reporting the time spend in throttling state */            \
        FILL_EVENT_HEADER(&message.header, THROTTLE_EVENT_TYPE, sizeof(struct event_message_t));   \
                                                                                                   \
        FILL_EVENT_PAYLOAD(&message.payload, INFO, 0, end_time - start_time, 0);                   \
                                                                                                   \
        /* call the callback function and post message */                                          \
        get_soc_power_reg()->event_cb(0, &message);                                                \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: event_cb is not initialized\r\n"); \
    }

/*! \def UPDATE_RESIDENCY(RESIDENCY_TYPE) 
    \brief Updates residency
*/
#define UPDATE_RESIDENCY(RESIDENCY_TYPE)                               \
    get_soc_power_reg()->RESIDENCY_TYPE.cumulative += time_usec;       \
                                                                       \
    /* Update the AVG throttle time */                                 \
    get_soc_power_reg()->RESIDENCY_TYPE.average =                      \
        (get_soc_power_reg()->RESIDENCY_TYPE.average + time_usec) / 2; \
                                                                       \
    /* Update the MAX throttle time */                                 \
    if (get_soc_power_reg()->RESIDENCY_TYPE.maximum < time_usec)       \
    {                                                                  \
        get_soc_power_reg()->RESIDENCY_TYPE.maximum = time_usec;       \
    }                                                                  \
                                                                       \
    /* Update the MIN throttle time */                                 \
    if (get_soc_power_reg()->RESIDENCY_TYPE.minimum > time_usec)       \
    {                                                                  \
        get_soc_power_reg()->RESIDENCY_TYPE.minimum = time_usec;       \
    }

/*! \def TDP_LEVEL_SAFE_GUARD
    \brief TDP level safe guard, (power_scale_factor)% above TDP level
*/
#define TDP_LEVEL_SAFE_GUARD \
    ((tdp_level_mW * (100 + (get_soc_power_reg()->power_scale_factor))) / 100)

/************************************************************************
*
*   FUNCTION
*
*       update_module_power_state
*
*   DESCRIPTION
*
*       This function updates the power state of the module.
*
*   INPUTS
*
*       power_state_e
*
*   OUTPUTS
*
*       int                 Return status
*
***********************************************************************/
int update_module_power_state(power_state_e state)
{
    if (power_state_change_time == 0)
    {
        power_state_change_time = timer_get_ticks_count();
    }
    else
    {
        update_module_power_residency(state, timer_get_ticks_count() - power_state_change_time);
        // TODO: Add tracing for power state changes
    }

    get_soc_power_reg()->module_power_state = state;
    power_state_change_time = timer_get_ticks_count();

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_power_state
*
*   DESCRIPTION
*
*       This function gets the module power state.
*
*   INPUTS
*
*       *power_state       Pointer to power state variable
*
*   OUTPUTS
*
*       int                 Return status
*
***********************************************************************/
int get_module_power_state(power_state_e *power_state)
{
    *power_state = get_soc_power_reg()->module_power_state;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_module_tdp_level
*
*   DESCRIPTION
*
*       This function updates the TDP level in the Global Power Register.
*
*   INPUTS
*
*       tdp_level_e
*
*   OUTPUTS
*
*       int                 Return status
*
***********************************************************************/
int update_module_tdp_level(uint8_t tdp)
{
    if (tdp > POWER_THRESHOLD_HI)
    {
        Log_Write(LOG_LEVEL_ERROR, "SW TDP can not be over HW power threshold!\n");
        return THERMAL_PWR_MGMT_INVALID_TDP_LEVEL;
    }
    else
    {
        Log_Write(LOG_LEVEL_DEBUG,
              "thermal pwr mgmt svc: Update module tdp level threshold to new level: %d\r\n", tdp);
        get_soc_power_reg()->module_tdp_level = tdp;
    }
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_tdp_level
*
*   DESCRIPTION
*
*       This function gets the TDP level of the module as per 
*       Global Power Register.
*
*   INPUTS
*
*       *tdp_level         Pointer to tdp level variable
*
*   OUTPUTS
*
*       int                 Return status
*
***********************************************************************/
int get_module_tdp_level(uint8_t *tdp_level)
{
    *tdp_level = get_soc_power_reg()->module_tdp_level;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_module_temperature_threshold
*
*   DESCRIPTION
*
*       This function updates the temperature threshold of the module.
*
*   INPUTS
*
*       threshold
*
*   OUTPUTS
*
*       int                 Return status
*
***********************************************************************/
int update_module_temperature_threshold(uint8_t lo_threshold)
{
    // TODO: Update field names to sw and hw threshold instead of lo and hi
    get_soc_power_reg()->temperature_threshold.lo_temperature_c = lo_threshold;

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_temperature_threshold
*
*   DESCRIPTION
*
*       This function gets the temperature thresholds of the module.
*
*   INPUTS
*
*       *temperature_threshold         Pointer to temperature threshold
*                                      variable
*
*   OUTPUTS
*
*       int                 Return status
*
***********************************************************************/
int get_module_temperature_threshold(struct temperature_threshold_t *temperature_threshold)
{
    *temperature_threshold = get_soc_power_reg()->temperature_threshold;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_module_current_temperature
*
*   DESCRIPTION
*
*       This function gets the current temperature of the module.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int                      Return status
*
***********************************************************************/
int update_module_current_temperature(void)
{
    int status = 0;
    uint8_t temperature;
    struct event_message_t message;
    struct temperature_threshold_t temperature_threshold;

    if (0 != pmic_get_temperature(&temperature))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get temperature\r\n");
        status = THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else
    {
        get_soc_power_reg()->soc_temperature = temperature;
        /* Update max temperature so far */
        if (get_soc_power_reg()->max_temp < temperature)
        {
            get_soc_power_reg()->max_temp = temperature;
        }
    }

    get_module_temperature_threshold(&temperature_threshold);

    if ((get_soc_power_reg()->soc_temperature) > (temperature_threshold.lo_temperature_c))
    {
        /* Switch power throttle state only if we are currently in lower priority throttle
        state */
        if (get_soc_power_reg()->power_throttle_state < THERMAL_THROTTLE_DOWN)
        {
            /* Do the thermal throttling */
            get_soc_power_reg()->power_throttle_state = THERMAL_THROTTLE_DOWN;
            xTaskNotify(t_handle, 0, eSetValueWithOverwrite);
        }

        /* add details in message header and fill payload */
        FILL_EVENT_HEADER(&message.header, THERMAL_LOW,
                          sizeof(struct event_message_t) - sizeof(struct cmn_header_t));

        FILL_EVENT_PAYLOAD(&message.payload, WARNING, 0, get_soc_power_reg()->soc_temperature, 0);

        if (0 != get_soc_power_reg()->event_cb)
        {
            /* call the callback function and post message */
            get_soc_power_reg()->event_cb(UNCORRECTABLE, &message);
        }
        else
        {
            MESSAGE_ERROR("thermal pwr mgmt svc error: event_cb is not initialized\r\n");
            status = THERMAL_PWR_MGMT_EVENT_NOT_INITIALIZED;
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_current_temperature
*
*   DESCRIPTION
*
*       This function gets the current temperature of the module.
*
*   INPUTS
*
*       *soc_temperature         Pointer to temperature variable
*
*   OUTPUTS
*
*       int                      Return status
*
***********************************************************************/
int get_module_current_temperature(uint8_t *soc_temperature)
{
    uint8_t temperature;
    if (0 != pmic_get_temperature(&temperature))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get temperature\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else
    {
        get_soc_power_reg()->soc_temperature = temperature;
    }

    *soc_temperature = get_soc_power_reg()->soc_temperature;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_module_soc_power
*
*   DESCRIPTION
*
*       This function gets the current power consumed by the device.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int                Return status
*
***********************************************************************/
int update_module_soc_power(void)
{
    uint8_t soc_pwr;
    int32_t soc_pwr_mW;
    int32_t tdp_level_mW;

    if (0 != pmic_read_soc_power(&soc_pwr))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get soc power\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else
    {
        get_soc_power_reg()->soc_power = soc_pwr;
    }

    soc_pwr_mW = Power_Convert_Hex_to_mW(soc_pwr);
    tdp_level_mW = Power_Convert_Hex_to_mW(get_soc_power_reg()->module_tdp_level);

    if ((soc_pwr_mW > TDP_LEVEL_SAFE_GUARD) &&
        (get_soc_power_reg()->power_throttle_state < POWER_THROTTLE_DOWN))
    {
        /* Update the power state */
        if (get_soc_power_reg()->module_power_state != POWER_STATE_MAX_POWER &&
            get_soc_power_reg()->module_power_state != POWER_STATE_SAFE_POWER)
        {
            update_module_power_state(POWER_STATE_MAX_POWER);
        }

        /* Do the power throttling down */
        get_soc_power_reg()->power_throttle_state = POWER_THROTTLE_DOWN;
        xTaskNotify(t_handle, 0, eSetValueWithOverwrite);
    }
    else
    {
        /* Update the power state */
        if (get_soc_power_reg()->module_power_state != POWER_STATE_MANAGED_POWER &&
            get_soc_power_reg()->module_power_state != POWER_STATE_SAFE_POWER)
        {
            update_module_power_state(POWER_STATE_MANAGED_POWER);
        }
    }

    if ((soc_pwr_mW < tdp_level_mW) &&
        (get_soc_power_reg()->power_throttle_state < POWER_THROTTLE_UP))
    {
        /* Do the power throttling up */
        get_soc_power_reg()->power_throttle_state = POWER_THROTTLE_UP;
        xTaskNotify(t_handle, 0, eSetValueWithOverwrite);
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_soc_power
*
*   DESCRIPTION
*
*       This function gets the current power consumed by the device.
*
*   INPUTS
*
*       *soc_power         Pointer to soc power variable
*
*   OUTPUTS
*
*       int                Return status
*
***********************************************************************/
int get_module_soc_power(uint8_t *soc_power)
{
    *soc_power = get_soc_power_reg()->soc_power;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_voltage
*
*   DESCRIPTION
*
*       This function gets the voltage value for a given voltage domain.
*
*   INPUTS
*
*       *module_voltage         Pointer to Module voltage struct
*        shire                  The shire specific voltage domain
*
*   OUTPUTS
*
*       int                     Return status
*
***********************************************************************/
int get_module_voltage(struct module_voltage_t *module_voltage)
{
    uint8_t voltage;

    if (0 != pmic_get_voltage(DDR, &voltage))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get ddr voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else
    {
        get_soc_power_reg()->module_voltage.ddr = voltage;
    }

    if (0 != pmic_get_voltage(L2CACHE, &voltage))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get l2 cache voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else
    {
        get_soc_power_reg()->module_voltage.l2_cache = voltage;
    }

    if (0 != pmic_get_voltage(MAXION, &voltage))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get maxion voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else
    {
        get_soc_power_reg()->module_voltage.maxion = voltage;
    }

    if (0 != pmic_get_voltage(MINION, &voltage))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get minion voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else
    {
        get_soc_power_reg()->module_voltage.minion = voltage;
    }

    if (0 != pmic_get_voltage(PCIE, &voltage))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get pcie voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else
    {
        get_soc_power_reg()->module_voltage.pcie = voltage;
    }

    if (0 != pmic_get_voltage(NOC, &voltage))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get noc voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else
    {
        get_soc_power_reg()->module_voltage.noc = voltage;
    }

    if (0 != pmic_get_voltage(PCIE_LOGIC, &voltage))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get pcie logic voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else
    {
        get_soc_power_reg()->module_voltage.pcie_logic = voltage;
    }

    if (0 != pmic_get_voltage(VDDQLP, &voltage))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get vddqlp voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else
    {
        get_soc_power_reg()->module_voltage.vddqlp = voltage;
    }

    if (0 != pmic_get_voltage(VDDQ, &voltage))
    {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get vddq voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else
    {
        get_soc_power_reg()->module_voltage.vddq = voltage;
    }

    *module_voltage = get_soc_power_reg()->module_voltage;

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*
*       get_soc_max_temperature
*
*   DESCRIPTION
*
*       This function gets module's max temperature
*
*   INPUTS
*
*       *max_temp   Pointer to max temp variable.
*
*   OUTPUTS
*
*       int                     Return status
*
***********************************************************************/
int get_soc_max_temperature(uint8_t *max_temp)
{
    *max_temp = get_soc_power_reg()->max_temp;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_module_uptime
*
*   DESCRIPTION
*
*       This function updates the cumulative time starting from Device
*       Reset.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int                     Return status
*
***********************************************************************/
int update_module_uptime()
{
    uint64_t module_uptime;
    uint64_t seconds;
    uint16_t day;
    uint8_t hours;
    uint8_t minutes;

    // Module Uptime //
    module_uptime = timer_get_ticks_count();

    seconds = (module_uptime / 1000000);
    day = (uint16_t)(seconds / (HOURS_IN_DAY * SECONDS_IN_HOUR));
    seconds = (seconds % (HOURS_IN_DAY * SECONDS_IN_HOUR));
    hours = (uint8_t)(seconds / SECONDS_IN_HOUR);

    seconds %= SECONDS_IN_HOUR;
    minutes = (uint8_t)(seconds / SECONDS_IN_MINUTE);

    get_soc_power_reg()->module_uptime.day = day;      //days
    get_soc_power_reg()->module_uptime.hours = hours;  //hours
    get_soc_power_reg()->module_uptime.mins = minutes; //mins;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_uptime
*
*   DESCRIPTION
*
*       This function gets the cumulative time starting from Device
*       Reset.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int                     Return status
*
***********************************************************************/
int get_module_uptime(struct module_uptime_t *module_uptime)
{
    *module_uptime = get_soc_power_reg()->module_uptime;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_module_throttle_time
*
*   DESCRIPTION
*
*       This function gets module's throttle time and
*       updates the global variable
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int                     Return status
*
***********************************************************************/
int update_module_throttle_time(power_throttle_state_e throttle_state, uint64_t time_usec)
{
    switch (throttle_state)
    {
        case POWER_THROTTLE_UP: {
            UPDATE_RESIDENCY(power_up_throttled_states_residency)
            break;
        }
        case POWER_THROTTLE_DOWN: {
            UPDATE_RESIDENCY(power_down_throttled_states_residency)
            break;
        }
        case THERMAL_THROTTLE_DOWN: {
            UPDATE_RESIDENCY(thermal_down_throttled_states_residency)
            break;
        }
        case POWER_THROTTLE_SAFE: {
            UPDATE_RESIDENCY(power_safe_throttled_states_residency)
            break;
        }
        case THERMAL_THROTTLE_SAFE: {
            UPDATE_RESIDENCY(thermal_safe_throttled_states_residency)
            break;
        }
        default: {
            Log_Write(LOG_LEVEL_ERROR, "Unexpected power throtlling state!\n");
            return THERMAL_PWR_MGMT_UNKNOWN_THROTTLE_STATE;
        }
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_module_power_residency
*
*   DESCRIPTION
*
*       This function sets module's power residency and
*       updates the global variable
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int                     Return status
*
***********************************************************************/
int update_module_power_residency(power_state_e power_state, uint64_t time_usec)
{
    switch (power_state)
    {
        case POWER_STATE_MAX_POWER: {
            UPDATE_RESIDENCY(power_max_residency)
            break;
        }
        case POWER_STATE_MANAGED_POWER: {
            UPDATE_RESIDENCY(power_managed_residency)
            break;
        }
        case POWER_STATE_SAFE_POWER: {
            UPDATE_RESIDENCY(power_safe_residency)
            break;
        }
        case POWER_STATE_LOW_POWER: {
            UPDATE_RESIDENCY(power_low_residency)
            break;
        }
        default: {
            Log_Write(LOG_LEVEL_ERROR, "Unexpected power state!\n");
            return THERMAL_PWR_MGMT_UNKNOWN_POWER_STATE;
        }
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_throttle_time
*
*   DESCRIPTION
*
*       This function gets throttle time from the global variable.
*
*   INPUTS
*
*       throttle_time       Module's throttle time
*
*   OUTPUTS
*
*       int                 Return status
*
***********************************************************************/
int get_throttle_time(uint64_t *throttle_time)
{
    *throttle_time = get_soc_power_reg()->thermal_down_throttled_states_residency.cumulative;
    // TODO: Add get functions for all throtlle states, update function to return residency_t
    //       type containing cumulative, average, maximum and minimum residency
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_max_throttle_time
*
*   DESCRIPTION
*
*       This function gets Max throttle time from the global variable.
*
*   INPUTS
*
*       *max_throttled_time       Module's max throttle time
*
*   OUTPUTS
*
*       int                       Return status
*
***********************************************************************/
int get_max_throttle_time(uint64_t *max_throttle_time)
{
    *max_throttle_time = get_soc_power_reg()->thermal_down_throttled_states_residency.maximum;
    // TODO: Remove this function once upper one is updated
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       set_power_event_cb
*
*   DESCRIPTION
*
*       This function sets power event callback variable.
*
*   INPUTS
*
*       event_cb                  event callback function ptr
*
*   OUTPUTS
*
*       int                       Return status
*
***********************************************************************/
int set_power_event_cb(dm_event_isr_callback event_cb)
{
    get_soc_power_reg()->event_cb = event_cb;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       init_thermal_pwr_mgmt_service
*
*   DESCRIPTION
*
*       This function initializes thermal and power management service
*
*   INPUTS
*
*       event_cb                  none
*
*   OUTPUTS
*
*       int                       Return status
*
***********************************************************************/
int init_thermal_pwr_mgmt_service(void)
{
    int status = 0;

    get_soc_power_reg()->power_throttle_state = POWER_IDLE;

    /* Create the power management task - use for throttling and DVFS */
    t_handle = xTaskCreateStatic(thermal_power_task_entry, "TT_TASK", TT_TASK_STACK_SIZE, NULL,
                                 TT_TASK_PRIORITY, g_pm_task, &g_staticTask_ptr);
    if (!t_handle)
    {
        MESSAGE_ERROR("Task Creation Failed: Failed to create power management task.\n");
        status = THERMAL_PWR_MGMT_TASK_CREATION_FAILED;
    }

    if (!status)
    {
        status = pmic_thermal_pwr_cb_init(pmic_isr_callback);
    }

    /* Set default parameters */
    status = update_module_temperature_threshold(TEMP_THRESHOLD_LO);

    if (!status)
    {
        status = update_module_power_state(POWER_STATE_LOW_POWER);
    }

    if (!status)
    {
        status = update_module_tdp_level(POWER_THRESHOLD_LO);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       reduce_minion_operating_point
*
*   DESCRIPTION
*
*       This function will algorithmically lower down frequency/voltage 
*       of the Minion Shire to compensate for Over-Temperature OR 
*       Over-Power operating regions
*
*   INPUTS
*
*       uint32_t                  Amount of Power to be reduced by (in mW)
*
*   OUTPUTS
*
*       int                       Return status
*
***********************************************************************/
int reduce_minion_operating_point(int32_t delta_power, struct trace_power_event_status_t *power_status)
{
    /* Compute delta freq to compensate for delta Power */
    int32_t delta_freq = delta_power * DP_per_Mhz;
    int32_t new_freq = Get_Minion_Frequency() - delta_freq;

    if (0 != Minion_Shire_Update_PLL_Freq(pll_freq_to_mode(new_freq)))
    {
        Log_Write(LOG_LEVEL_ERROR, "Failed to update minion frequency!\n");
        return THERMAL_PWR_MGMT_MINION_FREQ_UPDATE_FAILED;
    }

    int32_t new_voltage = Minion_Get_Voltage_Given_Freq(new_freq);
    if (new_voltage != get_soc_power_reg()->module_voltage.minion)
    {
        //NOSONAR Minion_Shire_Voltage_Update(new_voltage);
    }

    Update_Minion_Frequency_Global_Reg(new_freq);

    power_status->tgt_freq = (uint16_t)new_freq;
    power_status->tgt_voltage = (uint16_t)new_voltage;

    Trace_Power_Status(Trace_Get_SP_CB(), power_status);

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       increase_minion_operating_point
*
*   DESCRIPTION
*
*       This function will algorithmically increase frequency/voltage 
*       of the Minion Shire to hit most efficient operating point
*
*   INPUTS
*
*       uint32_t                  Amount of Power to be increased by (in mW)
*
*   OUTPUTS
*
*       int                       Return status
*
***********************************************************************/
int increase_minion_operating_point(int32_t delta_power, struct trace_power_event_status_t *power_status)
{
    /* Compute delta freq to compensate for delta Power */
    int32_t delta_freq = delta_power * DP_per_Mhz;
    int32_t new_freq = Get_Minion_Frequency() + delta_freq;

    int32_t new_voltage = Minion_Get_Voltage_Given_Freq(new_freq);
    if (new_voltage != get_soc_power_reg()->module_voltage.minion)
    {
        //NOSONAR Minion_Shire_Voltage_Update(new_voltage);
    }

    if (0 != Minion_Shire_Update_PLL_Freq(pll_freq_to_mode(new_freq)))
    {
        Log_Write(LOG_LEVEL_ERROR, "Failed to update minion frequency!\n");
        return THERMAL_PWR_MGMT_MINION_FREQ_UPDATE_FAILED;
    }

    Update_Minion_Frequency_Global_Reg(new_freq);

    power_status->tgt_freq = (uint16_t) new_freq;
    power_status->tgt_voltage = (uint16_t) new_voltage;

    Trace_Power_Status(Trace_Get_SP_CB(), power_status);

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       go_to_safe_state
*
*   DESCRIPTION
*
*       This function will switch frequency and voltage to 
*       safe predefined values
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int                       Return status
*
***********************************************************************/
int go_to_safe_state(power_state_e power_state, power_throttle_state_e throttle_state)
{
    uint8_t current_temperature = DEF_SYS_TEMP_VALUE;
    uint8_t current_power = 0;
    struct trace_power_event_status_t power_status =  { 0 };
    int32_t new_voltage = Minion_Get_Voltage_Given_Freq(SAFE_STATE_FREQUENCY);

    if (SAFE_STATE_FREQUENCY < Get_Minion_Frequency())
    {
        if (0 != Minion_Shire_Update_PLL_Freq(pll_freq_to_mode(SAFE_STATE_FREQUENCY)))
        {
            Log_Write(LOG_LEVEL_ERROR, "Failed to go to safe state!\n");
            return THERMAL_PWR_MGMT_MINION_FREQ_UPDATE_FAILED;
        }

        if (new_voltage != get_soc_power_reg()->module_voltage.minion)
        {
            //NOSONAR Minion_Shire_Voltage_Update(new_voltage);
        }
    }
    else if (SAFE_STATE_FREQUENCY > Get_Minion_Frequency())
    {
        if (new_voltage != get_soc_power_reg()->module_voltage.minion)
        {
            //NOSONAR Minion_Shire_Voltage_Update(new_voltage);
        }

        if (0 != Minion_Shire_Update_PLL_Freq(pll_freq_to_mode(SAFE_STATE_FREQUENCY)))
        {
            Log_Write(LOG_LEVEL_ERROR, "Failed to go to safe state!\n");
            return THERMAL_PWR_MGMT_MINION_FREQ_UPDATE_FAILED;
        }
    }

    Update_Minion_Frequency_Global_Reg(SAFE_STATE_FREQUENCY);

    /* Update the power state */
    if (get_soc_power_reg()->module_power_state != POWER_STATE_SAFE_POWER)
    {
        update_module_power_state(POWER_STATE_SAFE_POWER);
    }

     /* Get the current power */
     if (0 != pmic_read_soc_power(&current_power))
     {
         Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get soc power\r\n");
     }
    
    if (0 != pmic_get_temperature(&current_temperature))
    {
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get temperature\r\n");
    }

    FILL_POWER_STATUS(power_status, throttle_state, power_state,
                      current_power, current_temperature, 
                      (uint16_t)SAFE_STATE_FREQUENCY, (uint16_t)new_voltage)

    Trace_Power_Status(Trace_Get_SP_CB(), &power_status);

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       power_throttling
*
*   DESCRIPTION
*
*       This function handles power throttling
*
*   INPUTS
*
*       throttle_state
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void power_throttling(power_throttle_state_e throttle_state)
{
    uint64_t start_time;
    uint64_t end_time;
    uint8_t current_temperature = 0;
    uint8_t current_power = 0;
    int32_t current_power_mW;
    int32_t tdp_level_mW;
    int32_t delta_power_mW;
    uint8_t throttle_condition_met = 0;
    struct trace_power_event_status_t power_status = { 0 };

    /* We need to throttle the voltage and frequency, lets keep track of throttling time */
    start_time = timer_get_ticks_count();

    if (throttle_state == POWER_THROTTLE_SAFE)
    {
        go_to_safe_state(get_soc_power_reg()->module_power_state, throttle_state);
    }

    if (0 != pmic_get_temperature(&current_temperature))
    {
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get soc temperature\r\n");
    }

    if (0 != pmic_read_soc_power(&current_power))
    {
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get soc power\r\n");
    }

    current_power_mW = Power_Convert_Hex_to_mW(current_power);
    tdp_level_mW = Power_Convert_Hex_to_mW(get_soc_power_reg()->module_tdp_level);

    while (!throttle_condition_met &&
           (get_soc_power_reg()->power_throttle_state <= throttle_state))
    {
        /* Program the new operating point */
        switch (throttle_state)
        {
            case POWER_THROTTLE_UP: {
                delta_power_mW = ((current_power_mW * (get_soc_power_reg()->power_scale_factor)) / 100);
                FILL_POWER_STATUS(power_status, throttle_state, get_soc_power_reg()->module_power_state, 
                                  current_power, current_temperature, 0, 0)
                increase_minion_operating_point(delta_power_mW, &power_status);
                break;
            }
            case POWER_THROTTLE_DOWN: {
                delta_power_mW = ((current_power_mW * (get_soc_power_reg()->power_scale_factor)) / 100);
                FILL_POWER_STATUS(power_status, throttle_state, get_soc_power_reg()->module_power_state, 
                                  current_power, current_temperature, 0, 0)
                reduce_minion_operating_point(delta_power_mW, &power_status);
                break;
            }
            case POWER_THROTTLE_SAFE: {
                break;
            }
            default: {
                Log_Write(LOG_LEVEL_ERROR, "Unexpected power throtlling state!\n");
            }
        }

        /* TODO: What should be this delay? */
        vTaskDelay(pdMS_TO_TICKS(DELTA_TEMP_UPDATE_PERIOD));

        /* Get the current power */
        if (0 != pmic_read_soc_power(&current_power))
        {
            Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get soc power\r\n");
        }
        current_power_mW = Power_Convert_Hex_to_mW(current_power);

        /* Check if throttle condition is met */
        switch (throttle_state)
        {
            case POWER_THROTTLE_UP: {
                if (current_power_mW > tdp_level_mW)
                {
                    throttle_condition_met = 1;
                }
                break;
            }
            case POWER_THROTTLE_DOWN:
            case POWER_THROTTLE_SAFE: {
                if (current_power_mW < TDP_LEVEL_SAFE_GUARD)
                {
                    throttle_condition_met = 1;
                }
                break;
            }
            default: {
                Log_Write(LOG_LEVEL_ERROR, "Unexpected power throtlling state!\n");
            }
        }
    }

    end_time = timer_get_ticks_count();

    if (get_soc_power_reg()->power_throttle_state == throttle_state)
    {
        get_soc_power_reg()->power_throttle_state = POWER_IDLE;
    }

    /* Update the power state */
    if (get_soc_power_reg()->module_power_state != POWER_STATE_MANAGED_POWER)
    {
        update_module_power_state(POWER_STATE_MANAGED_POWER);
    }

    /* Update the throttle time here */
    update_module_throttle_time(throttle_state, end_time - start_time);
}

/************************************************************************
*
*   FUNCTION
*
*       thermal_throttling
*
*   DESCRIPTION
*
*       This function handles thermal throttling
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void thermal_throttling(power_throttle_state_e throttle_state)
{
    uint64_t start_time;
    uint64_t end_time;
    uint8_t current_temperature = DEF_SYS_TEMP_VALUE;
    struct event_message_t message;
    uint8_t current_power = 0;
    int32_t current_power_mW;
    int32_t delta_power_mW;
    struct trace_power_event_status_t power_status = { 0 };

    /* We need to throttle the voltage and frequency, lets keep track of throttling time */
    start_time = timer_get_ticks_count();

    if (throttle_state == THERMAL_THROTTLE_SAFE)
    {
        go_to_safe_state(get_soc_power_reg()->module_power_state, throttle_state);
    }

    if (0 != pmic_get_temperature(&current_temperature))
    {
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get temperature\r\n");
    }

    while ((current_temperature > get_soc_power_reg()->temperature_threshold.lo_temperature_c) &&
           (get_soc_power_reg()->power_throttle_state <= THERMAL_THROTTLE_DOWN))
    {
        switch (throttle_state)
        {
            case THERMAL_THROTTLE_DOWN: {
                /* Get the current power */
                if (0 != pmic_read_soc_power(&current_power))
                {
                    Log_Write(LOG_LEVEL_ERROR,
                              "thermal pwr mgmt svc error: failed to get soc power\r\n");
                }

                current_power_mW = Power_Convert_Hex_to_mW(current_power);
                delta_power_mW = ((current_power_mW * (get_soc_power_reg()->power_scale_factor)) / 100);

                FILL_POWER_STATUS(power_status, throttle_state, get_soc_power_reg()->module_power_state, 
                                  current_power, current_temperature, 0, 0)
                /* Program the new operating point  */
                reduce_minion_operating_point(delta_power_mW, &power_status);
                break;
            }
            case THERMAL_THROTTLE_SAFE: {
                break;
            }
            default: {
                Log_Write(LOG_LEVEL_ERROR, "Unexpected power throtlling state!\n");
            }
        }

        /* TODO: What should be this delay? */
        vTaskDelay(pdMS_TO_TICKS(DELTA_TEMP_UPDATE_PERIOD));

        /* Sample the temperature again */
        if (0 != pmic_get_temperature(&current_temperature))
        {
            Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get temperature\r\n");
        }
    }

    end_time = timer_get_ticks_count();

    if (get_soc_power_reg()->power_throttle_state == THERMAL_THROTTLE_DOWN)
    {
        get_soc_power_reg()->power_throttle_state = THERMAL_IDLE;
    }

    SEND_THROTTLE_EVENT(THROTTLE_TIME);

    /* Update the throttle time here */
    update_module_throttle_time(THERMAL_THROTTLE_DOWN, end_time - start_time);
}

/************************************************************************
*
*   FUNCTION
*
*       thermal_power_task_entry
*
*   DESCRIPTION
*
*       Entry function for power mgmt task
*
*   INPUTS
*
*       pvParameter                Task parameter
*
*   OUTPUTS
*
*       void                       none
*
***********************************************************************/
void thermal_power_task_entry(void *pvParameter)
{
    uint32_t notificationValue;

    (void)pvParameter;

    while (1)
    {
        if (get_soc_power_reg()->power_throttle_state < POWER_THROTTLE_UP)
        {
            xTaskNotifyWait(0, 0xFFFFFFFFU, &notificationValue, portMAX_DELAY);
        }

        Log_Write(LOG_LEVEL_INFO, "Received PMIC event: %s\n", __func__);

        switch (get_soc_power_reg()->power_throttle_state)
        {
            case POWER_THROTTLE_UP: {
                power_throttling(POWER_THROTTLE_UP);
                break;
            }
            case POWER_THROTTLE_DOWN: {
                power_throttling(POWER_THROTTLE_DOWN);
                break;
            }
            case THERMAL_THROTTLE_DOWN: {
                thermal_throttling(THERMAL_THROTTLE_DOWN);
                break;
            }
            case POWER_THROTTLE_SAFE: {
                power_throttling(POWER_THROTTLE_SAFE);
                break;
            }
            case THERMAL_THROTTLE_SAFE: {
                thermal_throttling(THERMAL_THROTTLE_SAFE);
                break;
            }
            default: {
                Log_Write(LOG_LEVEL_ERROR, "Unexpected power throtlling state!\n");
                break;
            }
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_isr_callback
*
*   DESCRIPTION
*
*       Callback function invoked from PMIC ISR
*
*   INPUTS
*
*       type                  Error type
*       msg                   Error/Event message
*
*   OUTPUTS
*
*       void                       none
*
***********************************************************************/
static void pmic_isr_callback(enum error_type type, struct event_message_t *msg)
{
    (void)type;
    BaseType_t xHigherPriorityTaskWoken;

    switch (msg->payload.syndrome[0])
    {
        case PMIC_INT_CAUSE_OVER_POWER: /* POWER CRITICAL PMIC CALLBACK */
        {
            get_soc_power_reg()->power_throttle_state = POWER_THROTTLE_SAFE;
            break;
        }
        case PMIC_INT_CAUSE_OVER_TEMP: /* THERMAL CRITICAL PMIC CALLBACK */
        {
            get_soc_power_reg()->power_throttle_state = THERMAL_THROTTLE_SAFE;
            break;
        }
        default: {
            Log_Write(LOG_LEVEL_ERROR, "Unexpected PMIC callback event!\n");
            break;
        }
    }

    xTaskNotifyFromISR(t_handle, (uint32_t)msg->header.msg_id, eSetValueWithOverwrite,
                       &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/************************************************************************
*
*   FUNCTION
*
*       dump_power_globals
*
*   DESCRIPTION
*
*       Dumps important power and thermal globals
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void dump_power_globals(void)
{
    volatile struct soc_power_reg_t const *soc_power_reg = get_soc_power_reg();

    /* Dump power mgmt globals */
    Log_Write(
        LOG_LEVEL_CRITICAL,
        "power_state = %u, tdp_level = %u, temperature = %u c, power = %u W, max_temperature = %u c\n",
        soc_power_reg->module_power_state, soc_power_reg->module_tdp_level,
        soc_power_reg->soc_temperature, soc_power_reg->soc_power, soc_power_reg->max_temp);

    Log_Write(LOG_LEVEL_CRITICAL, "Module uptime (day:hours:mins): %d:%d:%d\n",
              soc_power_reg->module_uptime.day, soc_power_reg->module_uptime.hours,
              soc_power_reg->module_uptime.mins);

    Log_Write(LOG_LEVEL_CRITICAL, "Module throttled residency = %lu\n",
              soc_power_reg->thermal_down_throttled_states_residency.cumulative);
    // TODO: Add log write for all throttle states

    Log_Write(
        LOG_LEVEL_CRITICAL,
        "Module Voltages (mV) :  ddr = %u , l2_cache = %u, maxion = %u, minion = %u, pcie = %u, noc = %u, pcie_logic = %u, vddqlp = %u, vddq = %u\n",
        soc_power_reg->module_voltage.ddr, soc_power_reg->module_voltage.l2_cache,
        soc_power_reg->module_voltage.maxion, soc_power_reg->module_voltage.minion,
        soc_power_reg->module_voltage.pcie, soc_power_reg->module_voltage.noc,
        soc_power_reg->module_voltage.pcie_logic, soc_power_reg->module_voltage.vddqlp,
        soc_power_reg->module_voltage.vddq);
}
