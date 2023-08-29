#ifndef __THERMAL_POWER_H__
#define __THERMAL_POWER_H__
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
#include <stdlib.h>
#include "dm.h"
#include "bl2_pmic_controller.h"
#include "bl2_pvt_controller.h"
#include "dm_event_def.h"
#include "trace.h"

// Thresholds
#define L0 0x0 // Low
#define HI 0x1 // High

// Default Reset values
#define DEF_SYS_TEMP_VALUE 52 // Expected average temperature

// Defines for uptime calc
#define HOURS_IN_DAY      24
#define SECONDS_IN_HOUR   3600
#define SECONDS_IN_MINUTE 60

// Defines the Boot voltages for the respective Voltage Domains
#define MINION_BOOT_VOLTAGE 0x32U // 500 mV
#define SRAM_BOOT_VOLTAGE   0x64U // 730 mV
#define NOC_BOOT_VOLTAGE    0x2FU // 485 mV
#define DDR_BOOT_VOLTAGE    0x6EU // 800 mV
#define MXN_BOOT_VOLTAGE    0x46U // 600 mV

/* define to convert Hex value to millivolt*/
#define MINION_HEX_TO_MILLIVOLT(hex_val)                                                     \
    PMIC_HEX_TO_MILLIVOLT(hex_val, PMIC_MINION_VOLTAGE_BASE, PMIC_MINION_VOLTAGE_MULTIPLIER, \
                          PMIC_GENERIC_VOLTAGE_DIVIDER)

#define SRAM_HEX_TO_MILLIVOLT(hex_val)                                                   \
    PMIC_HEX_TO_MILLIVOLT(hex_val, PMIC_SRAM_VOLTAGE_BASE, PMIC_SRAM_VOLTAGE_MULTIPLIER, \
                          PMIC_GENERIC_VOLTAGE_DIVIDER)

/*!
    \def MNN_BOOT_FREQUENCY
    \brief Defines the boot frequency. This constant represents the boot frequency in Mhz.
*/
#define MNN_BOOT_FREQUENCY 600U

/*!
    \def SRAM_BOOT_FREQUENCY
    \brief Defines the SRAM boot frequency. Minion and SRAM uses same clock so the frequncy is
           same for both modules.
*/
#define SRAM_BOOT_FREQUENCY MNN_BOOT_FREQUENCY

/* minion frequency limits */
#define MINION_FREQUENCY_MAX_LIMIT  700
#define MINION_FREQUENCY_MIN_LIMIT  300
#define MINION_FREQUENCY_STEP_VALUE 50

/* minion voltage limits */
#define MINION_VOLTAGE_MIN_LIMIT 400
#define MINION_VOLTAGE_MAX_LIMIT 650

/* L2CACHE voltage limits */
#define L2CACHE_VOLTAGE_MIN_LIMIT 650
#define L2CACHE_VOLTAGE_MAX_LIMIT 1000

// Defines for converting power values
#define POWER_10MW_TO_MW(pwr_10mw) (pwr_10mw * 10)
#define POWER_10MW_TO_W(pwr_10mw)  (pwr_10mw / 100)

/*! \def TEMP_THRESHOLD_SW_MANAGED
    \brief A macro that provides early indication temperature threshold value
*/
#define TEMP_THRESHOLD_SW_MANAGED 65

/*! \def POWER_THRESHOLD_SW_MANAGED
    \brief A macro that provides early indication power threshold value.
*          This power threshold will be set as initial value for SW power managing.
*          Following M.2 spec.
*/
#define POWER_THRESHOLD_SW_MANAGED 65

/*! \def SAFE_POWER_THRESHOLD
    \brief A macro that provides safe power threshold value.
*          When catastrophic overpower or overtemperature occures ETSOC will go bellow
*          this power threshold.
*          Following M.2 spec.
*/
#define SAFE_POWER_THRESHOLD 30

/*! \def SAFE_STATE_FREQUENCY
    \brief A macro that provides safe state frequency
*/
#define SAFE_STATE_FREQUENCY 300

/*! \def POWER_GUARDBAND_SCALE_FACTOR
    \brief A macro that provides power scale factor in percentages.
*/
#define POWER_GUARDBAND_SCALE_FACTOR 5

/*! \def DELTA_TEMP_UPDATE_PERIOD
    \brief A macro that provides dTj/dt - time(uS) for Junction temperature to be updated
           This value needs to be characterized in Silicon
*/
#define DELTA_TEMP_UPDATE_PERIOD 1000

/* define to convert power to milliwats*/
#define POWER_IN_MW(pwr) (pwr * 1000)

/*! \fn volatile struct soc_power_reg_t *get_soc_power_reg(void)
    \brief Interface to get the SOC power register
    \param none
    \returns Returns pointer to SOC power reg struct
*/
volatile struct soc_power_reg_t *get_soc_power_reg(void);

/*! \fn volatile struct pmic_power_reg_t *get_pmic_power_reg(void)
    \brief Interface to get the PMIC power register
    \param none
    \returns Returns pointer to PMIC power reg struct
*/
volatile struct pmic_power_reg_t *get_pmic_power_reg(void);

/*! \fn int set_module_active_power_management(active_power_management_e state)
    \brief Interface to update the module Active Power Management.
    \param state   PActive Power Management
    \returns Status indicating success or negative error
*/
int set_module_active_power_management(active_power_management_e state);

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

/*! \fn int update_module_tdp_level(uint8_t tdp)
    \brief Interface to update the module tdp level.
    \param tdp   TDP Level
    \returns Status indicating success or negative error
*/
int update_module_tdp_level(uint8_t tdp);

/*! \fn int get_module_tdp_level(uint8_t *tdp_level)
    \brief Interface to get the module tdp level.
    \param tdp_level*  Pointer to TDP Level
    \returns Status indicating success or negative error
*/
int get_module_tdp_level(uint8_t *tdp_level);

/*! \fn int update_module_temperature_threshold(uint8_t lo_threshold)
    \brief Interface to update the temperature threshold.
    \param threshold  Threshold for temperature
    \returns Status indicating success or negative error
*/
int update_module_temperature_threshold(uint8_t lo_threshold);

/*! \fn int get_module_temperature_threshold(struct temperature_threshold_t *temperature_threshold)
    \brief Interface to get the temperature threshold.
    \param temperature_threshold  Pointer to temperature threshold
    \returns Status indicating success or negative error
*/
int get_module_temperature_threshold(struct temperature_threshold_t *temperature_threshold);

/*! \fn int update_module_current_temperature(void)
    \brief Interface to update the module's current temperature.
    \param none
    \returns Status indicating success or negative error
*/
int update_module_current_temperature(void);

/*! \fn int get_module_current_temperature(struct current_temperature_t *temperature)
    \brief Interface to get the module's current temperature.
    \param temperature  Pointer to SOC temperature variable
    \returns Status indicating success or negative error
*/
int get_module_current_temperature(struct current_temperature_t *temperature);

/*! \fn int update_module_soc_power(void)
    \brief Interface to update the module's SOC Power consumption.
    \param none
    \returns Status indicating success or negative error
*/
int update_module_soc_power(void);

/*! \fn int update_module_frequencies(void)
    \brief Interface to update the module's frequency values in op stats.
    \param none
    \returns Status indicating success or negative error
*/
int update_module_frequencies(void);

/*! \fn int get_module_soc_power(uint16_t *soc_pwr_10mw)
    \brief Interface to get the module's SOC Power in 10 mW steps.
    \param soc_pwr_10mw Pointer to SOC power variable
    \returns Status indicating success or negative error
*/
int get_module_soc_power(uint16_t *soc_pwr_10mw);

/*! \fn int get_module_voltage(struct module_voltage_t *module_voltage)
    \brief Interface to get the module's voltage for different domains from PMIC.
    \param module_voltage Pointer to module voltage struct
    \returns Status indicating success or negative error
*/
int get_module_voltage(struct module_voltage_t *module_voltage);

/*! \fn int get_asic_voltage(struct asic_voltage_t *asic_voltage)
    \brief Interface to get the module's voltage for different domains from PVT.
    \param asic_voltage Pointer to asic voltage struct
    \returns Status indicating success or negative error
*/
int get_asic_voltage(struct asic_voltage_t *asic_voltage);

/*! \fn int get_delta_voltage(module_e module_type, int8_t *delta_voltage)
    \brief Reads the latest voltage values and calculates delta between them.
    \param module_type Module type
    \param delta_voltage Pointer to delta voltage
    \returns Status indicating success or negative error
*/
int get_delta_voltage(module_e module_type, int8_t *delta_voltage);

/*! \fn int update_module_uptime(void)
    \brief Interface to update the module's uptime.
    \param none
    \returns Status indicating success or negative error
*/
int update_module_uptime(void);

/*! \fn int get_module_uptime(struct module_uptime_t  *module_uptime)
    \brief Interface to get the module's uptime.
    \param module_uptime Pointer to module uptime struct
    \returns Status indicating success or negative error
*/
int get_module_uptime(struct module_uptime_t *module_uptime);

/*! \fn int update_module_throttle_time(power_throttle_state_e throttle_state, uint64_t time_usec)
    \brief Interface to update the module's throttle time.
    \param throttle_state Throttle state for which residency is updated
    \param time_usec throttle time in usec
    \returns Status indicating success or negative error
*/
int update_module_throttle_time(power_throttle_state_e throttle_state, uint64_t time_msec);

/*! \fn int update_module_power_residency(power_state_e power_state, uint64_t time_msec)
    \brief Interface to update the module's power residency.
    \param power_state Power state for which residency is updated
    \param time_usec power residency in usec
    \returns Status indicating success or negative error
*/
int update_module_power_residency(power_state_e power_state, uint64_t time_msec);

/*! \fn int get_throttle_residency(power_throttle_state_e throttle_state,
*                                          struct residency_t *residency)
    \brief Interface to get the module throttle residency from the global variable
    \param power_state Power throttle state
    \param residency Pointer to residency variable
    \returns Status indicating success or negative error
*/
int get_throttle_residency(power_throttle_state_e throttle_state, struct residency_t *residency);

/*! \fn int get_power_residency(power_state_e power_state, struct residency_t *residency)
    \brief Interface to get the module power residency from the global variable
    \param power_state Power state
    \param residency Pointer to residency variable
    \returns Status indicating success or negative error
*/
int get_power_residency(power_state_e power_state, struct residency_t *residency);

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

/*! \fn void dump_power_globals(void)
    \brief This function prints the performance globals
    \param none
    \returns none
*/
void dump_power_globals(void);

/*! \fn void power_throttling(power_throttle_state_e throttle_state)
    \brief This function handles power throttling
    \param none
    \returns none
*/
void power_throttling(power_throttle_state_e throttle_state);

/*! \fn void trace_power_state_test(void *cmd)
    \brief TThis function logs power status in trace for test
    \param tag command tag
    \param req_start_time  Time stamp when the request was received by the Command Dispatcher
    \param cmd Command buffer
    \returns none
*/
void trace_power_state_test(uint16_t tag, uint64_t req_start_time, void *cmd);

/*! \fn void thermal_throttling(power_throttle_state_e throttle_state)
    \brief This function handles thermal throttling
    \param none
    \returns none
*/
void thermal_throttling(power_throttle_state_e throttle_state);

/*! \fn int update_pmb_stats(bool reset)
    \brief This function updates the PMB stats for NOC, SRAM and minion modules.
    \param reset reset stats before updating
    \returns Status indicating success or negative error
*/
int update_pmb_stats(bool reset);

/*! \fn void print_system_operating_point(void)
    \brief This function prints system operating point
    \param none
    \returns none
*/
void print_system_operating_point(void);

/*! \fn void set_system_voltages(void)
    \brief This function set the boot voltages for the main 3 supplies
    \param none
    \returns none
*/
void set_system_voltages(void);

/*! \fn void Thermal_Pwr_Mgmt_Get_Minion_Temperature(void)
    \brief This function returns minion temperature
    \param temp placeholder for temperature
    \returns Status indicating success or negative error
*/
int Thermal_Pwr_Mgmt_Get_Minion_Temperature(uint64_t *temp);

/*! \fn void Thermal_Pwr_Mgmt_Get_System_Temperature(void)
    \brief This function returns system operating temperature
    \param temp placeholder for temperature
    \returns Status indicating success or negative error
*/
int Thermal_Pwr_Mgmt_Get_System_Temperature(uint64_t *temp);

/*! \fn int Thermal_Pwr_Mgmt_Get_Minion_Power(uint64_t *power)
    \brief This function returns Minion operating power
    \param power placeholder for power value
    \returns Status indicating success or negative error
*/
int Thermal_Pwr_Mgmt_Get_Minion_Power(uint64_t *power);

/*! \fn int Thermal_Pwr_Mgmt_Get_NOC_Power(uint64_t *power)
    \brief This function returns NOC operating power
    \param power placeholder for power value
    \returns Status indicating success or negative error
*/
int Thermal_Pwr_Mgmt_Get_NOC_Power(uint64_t *power);

/*! \fn int Thermal_Pwr_Mgmt_Get_SRAM_Power(uint64_t *power)
    \brief This function returns SRAM operating power
    \param power placeholder for power value
    \returns Status indicating success or negative error
*/
int Thermal_Pwr_Mgmt_Get_SRAM_Power(uint64_t *power);

/*! \fn int Thermal_Pwr_Mgmt_Get_System_Power(uint64_t *power)
    \brief This function returns System operating power
    \param power placeholder for power value
    \returns Status indicating success or negative error
*/
int Thermal_Pwr_Mgmt_Get_System_Power(uint64_t *power);

/*! \fn int Thermal_Pwr_Mgmt_Get_System_Power_Temp_Stats(op_stats_t *stats)
    \brief This function returns System operating stats
    \param stats placeholder for stats value
    \returns Status indicating success or negative error
*/
int Thermal_Pwr_Mgmt_Get_System_Power_Temp_Stats(struct op_stats_t *stats);

/*! \fn int Thermal_Pwr_Mgmt_Init_OP_Stats(void)
    \brief This function initialize operaitng point stats to their current values.
    \returns Status indicating success or negative error
*/
int Thermal_Pwr_Mgmt_Init_OP_Stats(void);

/*! \fn Thermal_Pwr_Mgmt_Set_Validate_Voltage(module_e voltage_type, uint8_t voltage)
    \brief This function sets voltage through pmic and then check for voltage stability.
    \param voltage_type - voltage type to be set:
    \param voltage - voltage value to be set (binary encoded)
    \returns Status indicating success or negative error
*/
int Thermal_Pwr_Mgmt_Set_Validate_Voltage(module_e voltage_type, uint8_t voltage);

/*! \fn int pwr_svc_find_hpdpll_mode(uint16_t freq, uint8_t *hpdpll_mode)
    \brief This function finds the hdpll mode for a given frequency
    \param freq - frequency value for which hdpll mode is required
    \param hpdpll_mode - hdpll mode for given frequency
    \returns Status indicating success or negative error
*/
int pwr_svc_find_hpdpll_mode(uint16_t freq, uint8_t *hpdpll_mode);

/*! \fn void Thermal_Pwr_Mgmt_Update_MM_State(uint64_t state)
    \brief This function updates the MM kernel state variable
    \param None
    \returns None
*/
void Thermal_Pwr_Mgmt_Update_MM_State(uint64_t state);
#endif
