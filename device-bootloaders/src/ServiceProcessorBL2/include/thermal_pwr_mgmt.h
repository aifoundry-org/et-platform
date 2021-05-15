/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file thermal_pwr_mgmt.h
    \brief A C header that provides abstraction for Thermal and power manangement service's
    interfaces. These interfaces provide services using which
    the host can query device for thermal and power related details.
*/
/***********************************************************************/
#include "dm.h"
#include "bl2_pmic_controller.h"
#include "dm_event_def.h"

// Thresholds
#define L0 0x0 // Low
#define HI 0x1 // High

// Default Reset values
#define DEF_SYS_TEMP_VALUE 52 // Expected average temperature 

// Defines for uptime calc
#define HOURS_IN_DAY      24
#define SECONDS_IN_HOUR   3600
#define SECONDS_IN_MINUTE 60

/*! \fn volatile struct soc_power_reg_t *get_soc_power_reg(void)
    \brief Interface to get the SOC power register
    \param none
    \returns Returns pointer to SOC power reg struct
*/
volatile struct soc_power_reg_t *get_soc_power_reg(void);

/*! \fn int update_module_power_state(power_state_e state)
    \brief Interface to update the module power state.
    \param state   Power state
    \returns Status indicating success or negative error
*/
int update_module_power_state(power_state_e state);

/*! \fn int get_module_power_state(power_state_e *power_state)
    \brief Interface to get the module power state.
    \param *power_state  Pointer to power state variable
    \returns Status indicating success or negative error
*/
int get_module_power_state(power_state_e *power_state);

/*! \fn int update_module_tdp_level(tdp_level_e tdp)
    \brief Interface to update the module tdp level.
    \param tdp   TDP Level
    \returns Status indicating success or negative error
*/
int update_module_tdp_level(tdp_level_e tdp);

/*! \fn int get_module_tdp_level(tdp_level_e *tdp_level);
    \brief Interface to get the module tdp level.
    \param tdp_level*  Pointer to TDP Level
    \returns Status indicating success or negative error
*/
int get_module_tdp_level(tdp_level_e *tdp_level);

/*! \fn int update_module_temperature_threshold(uint8_t threshold);
    \brief Interface to update the temperature threshold.
    \param threshold  Threshold for temperature
    \returns Status indicating success or negative error
*/
int update_module_temperature_threshold(uint8_t threshold);

/*! \fn int get_module_temperature_threshold(struct temperature_threshold_t *temperature_threshold);
    \brief Interface to get the temperature threshold.
    \param temperature_threshold  Pointer to temperature threshold
    \returns Status indicating success or negative error
*/
int get_module_temperature_threshold(struct temperature_threshold_t *temperature_threshold);

/*! \fn int update_module_current_temperature(void);
    \brief Interface to update the module's current temperature.
    \param none
    \returns Status indicating success or negative error
*/
int update_module_current_temperature(void);

/*! \fn int get_module_current_temperature(uint8_t *soc_temperature);
    \brief Interface to get the module's current temperature.
    \param soc_temperature  Pointer to SOC temperature variable
    \returns Status indicating success or negative error
*/
int get_module_current_temperature(uint8_t *soc_temperature);

/*! \fn int update_module_soc_power(void);
    \brief Interface to update the module's SOC Power consumption.
    \param none
    \returns Status indicating success or negative error
*/
int update_module_soc_power(void);

/*! \fn int get_module_soc_power(uint8_t *soc_power);
    \brief Interface to get the module's SOC Power.
    \param soc_temperature  Pointer to SOC power variable
    \returns Status indicating success or negative error
*/
int get_module_soc_power(uint8_t *soc_power);

/*! \fn int get_module_voltage(struct module_voltage_t *module_voltage);
    \brief Interface to get the module's voltage for different domains.
    \param *module_voltage  Pointer to module voltage struct
    \returns Status indicating success or negative error
*/
int get_module_voltage(struct module_voltage_t *module_voltage);

/*! \fn void update_module_max_temp(void);
    \brief Interface to update the module's max temperature value.
    \param none
    \returns none
*/
void update_module_max_temp(void);

/*! \fn void update_module_uptime(void);
    \brief Interface to update the module's uptime.
    \param none
    \returns none
*/
void update_module_uptime(void);

/*! \fn int get_module_uptime(struct module_uptime_t  *module_uptime);
    \brief Interface to get the module's uptime.
    \param *module_uptime  Pointer to module uptime struct
    \returns Status indicating success or negative error
*/
int get_module_uptime(struct module_uptime_t *module_uptime);

/*! \fn void update_module_throttle_time(uint64_t time_usec);
    \brief Interface to update the module's throttle time.
    \param time_usec throttle time in usec
    \returns none
*/
void update_module_throttle_time(uint64_t time_msec);

/*! \fn int get_throttle_time(uint64_t *throttle_time)
    \brief Interface to get the module's throttle time.
    \param *throttle_time  Pointer to throttle time variable
    \returns Status indicating success or negative error
*/
int get_throttle_time(uint64_t *throttle_time);

/*! \fn int get_module_max_throttle_time(uint64_t *max_throttle_time)
    \brief Interface to get the module max throttle time from the global variable
    \param *max_throttle_time
    \returns Status indicating success or negative error
*/
int get_max_throttle_time(uint64_t *max_throttle_time);

/*! \fn int get_soc_max_temperature(uint8_t *max_temp)
    \brief Interface to get module max temperature from global variable
    \param *max_temp Pointer to max temp variable
    \returns Status indicating success or negative error
*/
int get_soc_max_temperature(uint8_t *max_temp);

/*! \fn int set_power_event_cb(dm_event_isr_callback event_cb)
    \brief Interface to set temperature event callback variable
    \param event_cb  Temperature event callback function ptr
    \returns Status indicating success or negative error
*/
int set_power_event_cb(dm_event_isr_callback event_cb);

/*! \fn int init_thermal_pwr_mgmt_service(void)
    \brief Initialization function
    \param none
    \returns Status indicating success or negative error
*/
int init_thermal_pwr_mgmt_service(void);
