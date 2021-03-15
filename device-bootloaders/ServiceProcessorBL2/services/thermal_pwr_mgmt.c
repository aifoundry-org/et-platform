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

*/
/***********************************************************************/
#include "thermal_pwr_mgmt.h"

struct soc_power_reg_t g_soc_power_reg __attribute__((section(".data")));

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
};

volatile struct soc_power_reg_t *get_soc_power_reg(void)
{
    return &g_soc_power_reg;
}

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
        printf("thermal pwr mgmt svc: update module tdp level error\r\n");
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
*       This function updates the temperature thresholds of the module.
*
*   INPUTS
*
*       hi_threshold
*       lo_threshold
*
*   OUTPUTS
*
*       int                 Return status
*
***********************************************************************/
int update_module_temperature_threshold(uint8_t hi_threshold, uint8_t lo_threshold)
{
    int status;

    status = pmic_set_temperature_threshold(L0, lo_threshold);

    if (0 != status) {
        printf("thermal pwr mgmt svc: set low temperature threshold error\r\n");
    } else {
        get_soc_power_reg()->temperature_threshold.lo_temperature_c = lo_threshold;

        status = pmic_set_temperature_threshold(HI, hi_threshold);

        if (0 != status) {
            printf("thermal pwr mgmt svc: set high temperature threshold error\r\n");
        } else {
            get_soc_power_reg()->temperature_threshold.hi_temperature_c = hi_threshold;
        }
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
    get_soc_power_reg()->soc_temperature = pmic_get_temperature();
    return 0;
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
    get_soc_power_reg()->soc_temperature = pmic_get_temperature();
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
    get_soc_power_reg()->soc_power = pmic_read_soc_power();
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
    get_soc_power_reg()->module_voltage.minion_shire_mV = (uint16_t)pmic_get_voltage(MODULE_MINION);
    get_soc_power_reg()->module_voltage.noc_mV = (uint16_t)pmic_get_voltage(MODULE_NOC);
    get_soc_power_reg()->module_voltage.pcie_shire_mV = (uint16_t)pmic_get_voltage(MODULE_PSHIRE);
    get_soc_power_reg()->module_voltage.io_shire_mV = (uint16_t)pmic_get_voltage(MODULE_IOSHIRE);
    get_soc_power_reg()->module_voltage.mem_shire_mV = (uint16_t)pmic_get_voltage(MODULE_MEMSHIRE);

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
    uint8_t curr_temp;

    curr_temp = pmic_get_temperature();

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
void update_module_throttle_time(void)
{
    // TODO : Set it to throttle time to valid value. Compute/Read from HW.
    // https://esperantotech.atlassian.net/browse/SW-6559
    // Update the value in get_soc_power_reg()->throttled_states_residency.
    get_soc_power_reg()->throttled_states_residency = 1000;

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