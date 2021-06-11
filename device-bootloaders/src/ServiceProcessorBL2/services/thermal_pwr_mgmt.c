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
#include "error.h"

struct soc_power_reg_t g_soc_power_reg __attribute__((section(".data")));


#define TT_TASK_STACK_SIZE 1024
#define TT_TASK_PRIORITY    1

/* The variable used to hold task's handle */
static TaskHandle_t t_handle;

/* Task entry function */
static void thermal_power_task_entry(void *pvParameters);

/* Task stack memory */
static StackType_t g_pm_task[TT_TASK_STACK_SIZE];

/* The variable used to hold the task's data structure. */
static StaticTask_t g_staticTask_ptr;


struct soc_power_reg_t {
    power_state_e module_power_state;
    tdp_level_e module_tdp_level;
    struct temperature_threshold_t temperature_threshold;
    uint8_t soc_temperature;
    uint8_t soc_power;
    uint8_t max_temp;
    struct module_uptime_t module_uptime;
    struct module_voltage_t module_voltage;
    uint64_t throttled_states_residency;
    uint64_t max_throttled_states_residency;
    dm_event_isr_callback event_cb;
};

volatile struct soc_power_reg_t *get_soc_power_reg(void)
{
    return &g_soc_power_reg;
}


// FIXME: This needs to extracted from OTP Fuse
#define DP_per_Mhz 1

// FIXME: Need to add Freq to mode convertion equation
#define FREQ2MODE(X) X

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
    // TODO : https://esperantotech.atlassian.net/browse/SW-6503
    get_soc_power_reg()->module_power_state = state;
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
*       This function updates the TDP level of the module.
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
int update_module_tdp_level(tdp_level_e tdp)
{
    int status;

    status = pmic_set_tdp_threshold(tdp);

    if (0 != status) {
        MESSAGE_ERROR("thermal pwr mgmt svc: update module tdp level error\r\n");
    } else {
        get_soc_power_reg()->module_tdp_level = tdp;
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_tdp_level
*
*   DESCRIPTION
*
*       This function gets the TDP level of the module.
*
*   INPUTS
*
*       *tdp_level         Pointer to tdp level variable
*
*   OUTPUTS
*
*       int                Return status
*
***********************************************************************/
int get_module_tdp_level(tdp_level_e *tdp_level)
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
int update_module_temperature_threshold(uint8_t threshold)
{
    int status = 0;

    if(0 != pmic_set_temperature_threshold(threshold)) {
        MESSAGE_ERROR("thermal pwr mgmt svc: set temperature threshold error\r\n");
        status = THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else {
        get_soc_power_reg()->temperature_threshold.temperature = threshold;
    }

    return status;
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
*       int                            Return status
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
    int status=0;
    uint8_t temperature;
    struct event_message_t message;
    struct temperature_threshold_t temperature_threshold;

    if(0 != pmic_get_temperature(&temperature)) {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get temperature\r\n");
        status = THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else {
        get_soc_power_reg()->soc_temperature = temperature;
    }

    get_module_temperature_threshold(&temperature_threshold);

    if ((get_soc_power_reg()->soc_temperature) > (temperature_threshold.temperature)) {
        /* add details in message header and fill payload */
        FILL_EVENT_HEADER(&message.header, THERMAL_LOW,
                          sizeof(struct event_message_t) - sizeof(struct cmn_header_t));

        FILL_EVENT_PAYLOAD(&message.payload, WARNING, 0, get_soc_power_reg()->soc_temperature, 0);

        if (0 != get_soc_power_reg()->event_cb) {
            /* call the callback function and post message */
            get_soc_power_reg()->event_cb(UNCORRECTABLE, &message);
        } else {
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
    if(0 != pmic_get_temperature(&temperature)) {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get temperature\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else {
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

    if(0 != pmic_read_soc_power(&soc_pwr)) {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get soc power\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else {
        get_soc_power_reg()->soc_power = soc_pwr;
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

    if(0 != pmic_get_voltage(DDR, &voltage)) {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get ddr voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else {
        get_soc_power_reg()->module_voltage.ddr = voltage;
    }

    if(0 != pmic_get_voltage(L2CACHE, &voltage)) {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get l2 cache voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else {
        get_soc_power_reg()->module_voltage.l2_cache = voltage;
    }

    if(0 != pmic_get_voltage(MAXION, &voltage)) {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get maxion voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else {
        get_soc_power_reg()->module_voltage.maxion = voltage;
    }

    if(0 != pmic_get_voltage(MINION, &voltage)) {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get minion voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else {
        get_soc_power_reg()->module_voltage.minion = voltage;
    }

    if(0 != pmic_get_voltage(PCIE, &voltage)) {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get pcie voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else {
        get_soc_power_reg()->module_voltage.pcie = voltage;
    }

    if(0 != pmic_get_voltage(NOC, &voltage)) {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get noc voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else {
        get_soc_power_reg()->module_voltage.noc = voltage;
    }

    if(0 != pmic_get_voltage(PCIE_LOGIC, &voltage)) {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get pcie logic voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else {
        get_soc_power_reg()->module_voltage.pcie_logic = voltage;
    }

    if(0 != pmic_get_voltage(VDDQLP, &voltage)) {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get vddqlp voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else {
        get_soc_power_reg()->module_voltage.vddqlp = voltage;
    }

    if(0 != pmic_get_voltage(VDDQ, &voltage)) {
        MESSAGE_ERROR("thermal pwr mgmt svc error: failed to get vddq voltage\r\n");
        return THERMAL_PWR_MGMT_PMIC_ACCESS_FAILED;
    }
    else {
        get_soc_power_reg()->module_voltage.vddq = voltage;
    }

    *module_voltage = get_soc_power_reg()->module_voltage;

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_module_max_temp
*
*   DESCRIPTION
*
*       This function updates the max temperature in global variable.
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
void update_module_max_temp(void)
{
    uint8_t curr_temp = DEF_SYS_TEMP_VALUE;

    if(0 != pmic_get_temperature(&curr_temp)) {
        Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get temperature\r\n");
    }

    if (get_soc_power_reg()->max_temp < curr_temp) {
        get_soc_power_reg()->max_temp = curr_temp;
    }
}

/************************************************************************
*
*   FUNCTION
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
*       int     max temperature value
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
*       None
*
***********************************************************************/
void update_module_uptime()
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

    get_soc_power_reg()->module_uptime.day = day; //days
    get_soc_power_reg()->module_uptime.hours = hours; //hours
    get_soc_power_reg()->module_uptime.mins = minutes; //mins;
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
*       None
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
*       None
*
***********************************************************************/
void update_module_throttle_time(uint64_t time_usec)
{
    // TODO : Set it to throttle time to valid value. Compute/Read from HW.
    // https://esperantotech.atlassian.net/browse/SW-6559
    // Update the value in get_soc_power_reg()->throttled_states_residency.
    get_soc_power_reg()->throttled_states_residency += time_usec;

    // Update the MAX throttle time
    if (get_soc_power_reg()->max_throttled_states_residency <
        get_soc_power_reg()->throttled_states_residency) {
        get_soc_power_reg()->max_throttled_states_residency =
            get_soc_power_reg()->throttled_states_residency;
    }
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
    *throttle_time = get_soc_power_reg()->throttled_states_residency;
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
    *max_throttle_time = get_soc_power_reg()->max_throttled_states_residency;
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

    /* Create the power management task - use for throttling and DVFS */
    t_handle = xTaskCreateStatic(thermal_power_task_entry, "TT_TASK", TT_TASK_STACK_SIZE,
                                    NULL, TT_TASK_PRIORITY, g_pm_task,
                                    &g_staticTask_ptr);
    if (!t_handle) {
        MESSAGE_ERROR("Task Creation Failed: Failed to create power management task.\n");
        status = THERMAL_PWR_MGMT_TASK_CREATION_FAILED;
    }

    /* Set default parameters */
    status = update_module_temperature_threshold(PMIC_TEMP_THRESHOLD_DEFAULT);

    if (!status)
    {
        status = update_module_power_state(POWER_STATE_LOWEST);
    }

    if (!status)
    {
        status = update_module_tdp_level(TDP_LEVEL_ONE);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       set_operating_point
*
*   DESCRIPTION
*
*       This function sets the new operating point based on the
*       given power value.
*
*   INPUTS
*
*       event_cb                  Power value to set
*
*   OUTPUTS
*
*       int                       Return status
*
***********************************************************************/
static int set_operating_point(uint8_t power)
{
    tdp_level_e tdp;
    get_module_tdp_level(&tdp);

    // FIXME: Add Binary Encoding to floatiing point: BCD2FP(power);
    // Use this value to compare against TDP which should also
    // be converted to a floatng point value

    //calculate Delta Power then re-compute new Minion Freq
    bool increase = (power > tdp) ? true: false;
    int delta_power = increase ? (power - tdp): (tdp - power);
    int delta_freq = (delta_power * DP_per_Mhz)*(increase? 1 : -1);

    uint32_t new_freq = (uint32_t)( Get_Minion_Frequency() + delta_freq );
    
    // FIXME: Need check to see if Voltage update is required for new
    //        frequency
    // Minion_Shire_Voltage_Update(voltage);
    // FIXME: Need to add error handling in case Minion was not able to be updated
    //        check for error, then return Error enum instead
    Minion_Shire_Update_PLL_Freq((uint8_t)FREQ2MODE(new_freq));

    Update_Minion_Frequency(new_freq);

    return 0;

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
    uint64_t start_time;
    uint64_t end_time;
    uint8_t current_temperature=DEF_SYS_TEMP_VALUE;
    struct event_message_t message;
    uint32_t notificationValue;
    uint8_t current_power=0;

    (void)pvParameter;

    while(1)
    {
        xTaskNotifyWait(0, 0xFFFFFFFFU, &notificationValue, portMAX_DELAY);
        if (notificationValue == PMIC_ERROR)
        {
            if(0 != pmic_get_temperature(&current_temperature)) {
                Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get temperature\r\n");
            }

            if (current_temperature < get_soc_power_reg()->temperature_threshold.temperature)
                continue;

            if (get_soc_power_reg()->event_cb)
            {
                /* Send the event to host for exceeding the high temperature threshold */
                FILL_EVENT_HEADER(&message.header, PMIC_ERROR,
                            sizeof(struct event_message_t) - sizeof(struct cmn_header_t));

                FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, 0, 0x1, current_temperature);

                /* Invoke the callback function and post message */
                get_soc_power_reg()->event_cb(0, &message);
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: event_cb is not initialized\n");
            }

            /* We need to throttle the voltage and frequency, lets keep track of throttling time */
            start_time = timer_get_ticks_count();

            do
            {
                /* Get the current power */
                if(0 != pmic_read_soc_power(&current_power)) {
                    Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get soc power\r\n");
                }

                /* FIXME: Scale it down by 10 % of current value and change the frequency accordingly
                Take care that current_power is binary encoded */
                current_power = (uint8_t)((current_power * 10) / 100);

                /* Program the new operating point  */
                set_operating_point(current_power);

                /* FIXME: What should be this delay? */
                vTaskDelay(pdMS_TO_TICKS(THERMAL_THROTTLE_SAMPLE_PERIOD));

                /* Sample the temperature again */
                if(0 != pmic_get_temperature(&current_temperature)) {
                    Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: failed to get temperature\r\n");
                }
            } while(current_temperature > get_soc_power_reg()->temperature_threshold.temperature);

            end_time = timer_get_ticks_count();

            if (get_soc_power_reg()->event_cb)
            {
                /* Send the event to the device reporting the time spend in throttling state */
                FILL_EVENT_HEADER(&message.header, THROTTLE_TIME, sizeof(struct event_message_t) - sizeof(struct cmn_header_t));

                FILL_EVENT_PAYLOAD(&message.payload, INFO, 0, end_time - start_time, 0);
            
                /* call the callback function and post message */
                get_soc_power_reg()->event_cb(0, &message);
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error: event_cb is not initialized\r\n");
            }

            /* Update the throttle time here */
            update_module_throttle_time(end_time - start_time);
        }
    }
}

