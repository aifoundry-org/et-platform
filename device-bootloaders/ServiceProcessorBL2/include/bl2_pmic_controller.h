#ifndef __BL2_PMIC_CONTROLLER_H__
#define __BL2_PMIC_CONTROLLER_H__

#include "dm_event_def.h"

#include "pmic_i2c.h"

enum voltage_type_t { DDR = 0, L2CACHE, MAXION, MINION, PCIE, NOC, PCIE_LOGIC, VDDQLP, VDDQ };

/*! \fn void setup_pmic(void)
    \brief This function initialize I2C connection.
*/
void setup_pmic(void);

/*! \fn int32_t pmic_error_control_init(dm_event_isr_callback event_cb)
    \brief This function setup error callback.
    \param event_cb - callback pointer
    \return The function call status, pass/fail.
*/
int32_t pmic_error_control_init(dm_event_isr_callback event_cb);

/*! \fn void pmic_error_isr(void)
    \brief PMIC interrupt routine.
*/
void pmic_error_isr(void);

/*! \fn int pmic_get_fw_version(uint8_t* fw_version)
    \brief This function reads Firmware Version register of PMIC.
    \param fw_version - value of Firmware Version register of PMIC.
    \return The function call status, pass/fail.
*/
int pmic_get_fw_version(uint8_t *fw_version);

/*! \fn int pmic_get_gpo_bit(uint8_t index, uint8_t* gpo_bit_value)
    \brief This function reads GPO Control register bit of PMIC.
    \param index - index of bit to be fetched
    \param gpo_bit_value - value of the bit
    \return The function call status, pass/fail.
*/
int pmic_get_gpo_bit(uint8_t index, uint8_t *gpo_bit_value);

/*! \fn int pmic_set_gpo_bit(uint8_t index)
    \brief This function sets GPO Control register bit of PMIC.
    \param index - index of bit to be set
    \return The function call status, pass/fail.
*/
int pmic_set_gpo_bit(uint8_t index);

/*! \fn int pmic_clear_gpo_bit(uint8_t index)
    \brief This function clears GPO Control register bit of PMIC.
    \param index - index of bit to be cleared
    \return The function call status, pass/fail.
*/
int pmic_clear_gpo_bit(uint8_t index);

/*! \fn int pmic_get_input_voltage(uint8_t* input_voltage)
    \brief This function reads Input Voltage register of PMIC.
    \param input_voltage - input voltage (binary encoded)
    \return The function call status, pass/fail.
*/
int pmic_get_input_voltage(uint8_t *input_voltage);

/*! \fn int pmic_get_temperature_threshold(uint8_t* temp_threshold)
    \brief This function reads Temperature Alarm Configuration register of PMIC.
    \param temp_threshold - temperature threshold value in Celsius.
    \return The function call status, pass/fail.
*/
int pmic_get_temperature_threshold(uint8_t *temp_threshold);

/*! \fn int pmic_set_temperature_threshold(uint8_t* temp_limit)
    \brief This function sets Temperature Alarm Configuration register of PMIC.
    \param temp_limit - temperature threshold value in Celsius.
    \return The function call status, pass/fail.
*/
int pmic_set_temperature_threshold(uint8_t temp_limit);

/*! \fn int pmic_get_temperature(uint8_t* sys_temp)
    \brief This function reads System Temperature register of PMIC.
    \param sys_temp - system temperature in Celsius.
    \return The function call status, pass/fail.
*/
int pmic_get_temperature(uint8_t *sys_temp);

/*! \fn int pmic_read_soc_power(uint8_t* soc_pwr)
    \brief This function returns soc input power.
    \param soc_pwr - value of Input Power (binary encoded).
    \return The function call status, pass/fail.
*/
int pmic_read_soc_power(uint8_t *soc_pwr);

/*! \fn int pmic_enable_etsoc_reset_after_perst(void)
    \brief This function enables a PERST event to also reset the ET-SOC.
    \return The function call status, pass/fail.
*/
int pmic_enable_etsoc_reset_after_perst(void);

/*! \fn int pmic_disable_etsoc_reset_after_perst(void)
    \brief This function disables a PERST event to also reset the ET-SOC.
    \return The function call status, pass/fail.
*/
int pmic_disable_etsoc_reset_after_perst(void);

/*! \fn int pmic_enable_wdog_reset(void)
    \brief This function enables the watchdog timer to reset the ET-SOC.
    \return The function call status, pass/fail.
*/
int pmic_enable_wdog_reset(void);

/*! \fn int pmic_disable_wdog_reset(void)
    \brief This function disables the watchdog timer to reset the ET-SOC.
    \return The function call status, pass/fail.
*/
int pmic_disable_wdog_reset(void);

/*! \fn int pmic_get_reset_cause(uint8_t* reset_cause)
    \brief This function reads Reset Causation register of PMIC.
    \param reset_cause - value of Reset Causation register of PMIC.
    \return The function call status, pass/fail.
*/
int pmic_get_reset_cause(uint8_t *reset_cause);

/*! \fn int pmic_get_voltage(enum voltage_type_t voltage_type, uint8_t* voltage)
    \brief This function returns specific voltage setting.
    \param voltage_type - voltage type to be read:
*   - DDR
*   - L2CACHE
*   - MAXION
*   - MINION
*   - PCIE
*   - NOC
*   - PCIE_LOGIC
*   - VDDQLP
*   - VDDQ
    \param voltage - voltage value (binary encoded)
    \return The function call status, pass/fail.
*/
int pmic_get_voltage(enum voltage_type_t voltage_type, uint8_t *voltage);

/*! \fn int pmic_set_voltage(enum voltage_type_t voltage_type, uint8_t voltage)
    \brief This function returns specific voltage setting.
    \param voltage_type - voltage type to be set:
*   - DDR
*   - L2CACHE
*   - MAXION
*   - MINION
*   - PCIE
*   - NOC
*   - PCIE_LOGIC
*   - VDDQLP
*   - VDDQ
    \param voltage - voltage value to be set (binary encoded)
    \return The function call status, pass/fail.
*/
int pmic_set_voltage(enum voltage_type_t voltage_type, uint8_t voltage);

/*! \fn int pmic_get_minion_group_voltage(uint8_t group_id, uint8_t* voltage)
    \brief This function returns minion group voltage setting.
    \param group_id - minion group (1-17) voltage to be read
    \param voltage - voltage value (binary encoded)
    \return The function call status, pass/fail.
*/
int pmic_get_minion_group_voltage(uint8_t group_id, uint8_t *voltage);

/*! \fn int pmic_set_minion_group_voltage(uint8_t group_id, uint8_t voltage)
    \brief This function writes minion group voltage setting.
    \param group_id - minion group (1-17) voltage to be set
    \param voltage - voltage value to be set (binary encoded)
    \return The function call status, pass/fail.
*/
int pmic_set_minion_group_voltage(uint8_t group_id, uint8_t voltage);

/*! \fn int pmic_enable_wdog_timer(void)
    \brief This function enables the watchdog timer.
    \return The function call status, pass/fail.
*/
int pmic_enable_wdog_timer(void);

/*! \fn int pmic_disable_wdog_timer(void)
    \brief This function disables the watchdog timer.
    \return The function call status, pass/fail.
*/
int pmic_disable_wdog_timer(void);

/*! \fn int pmic_enable_wdog_timeout_reset(void)
    \brief This function enables timeout to assert RESET along with PERST.
    \return The function call status, pass/fail.
*/
int pmic_enable_wdog_timeout_reset(void);

/*! \fn int pmic_disable_wdog_timeout_reset(void)
    \brief This function disables timeout to assert RESET, PERST only will be asserted.
    \return The function call status, pass/fail.
*/
int pmic_disable_wdog_timeout_reset(void);

/*! \fn int pmic_get_wdog_timeout_time(int* wdog_time)
    \brief This function gets watchdog timeout time.
    \param wdog_time - wdog timeout time in miliseconds
    \return The function call status, pass/fail.
*/
int pmic_get_wdog_timeout_time(uint32_t *wdog_time);

/*! \fn int int pmic_set_wdog_timeout_time(int timeout_time)
    \brief This function sets watchdog timeout time.
    \param timeout_time - wdog timeout time in miliseconds (multiple of 200ms)
    \return The function call status, pass/fail.
*/
int pmic_set_wdog_timeout_time(uint32_t timeout_time);

/*! \fn int pmic_get_tdp_threshold(uint8_t* power_limit)
    \brief This function gets power alarm point.
    \param power_limit - power alarm point (binary encoded)
    \return The function call status, pass/fail.
*/
int pmic_get_tdp_threshold(uint8_t *power_limit);

/*! \fn int pmic_get_tdp_threshold(uint8_t* power_limit)
    \brief This function sets power alarm point.
    \param power_alarm - power alarm point to be set (binary encoded)
    \return The function call status, pass/fail.
*/
int pmic_set_tdp_threshold(uint8_t power_alarm);

/*! \fn int pmic_force_shutdown(void)
    \brief This function forces a shutdown, only a power cycle will allow the system to turn on.
    \return The function call status, pass/fail.
*/
int pmic_force_shutdown(void);

/*! \fn int pmic_force_power_off_on(void)
    \brief This function force a power off/on cycle which shuts down and restarts the system.
    \return The function call status, pass/fail.
*/
int pmic_force_power_off_on(void);

/*! \fn int pmic_force_reset(void)
    \brief This function forces a RESET to the ET-SOC.
    \return The function call status, pass/fail.
*/
int pmic_force_reset(void);

/*! \fn int pmic_force_perst(void)
    \brief This function forces a PERST reset to the ET-SOC.
    \return The function call status, pass/fail.
*/
int pmic_force_perst(void);

/*! \fn int pmic_reset_wdog_timer(void)
    \brief This function resets watchdog timer.
    \return The function call status, pass/fail.
*/
int pmic_reset_wdog_timer(void);

/*! \fn int pmic_get_average_soc_power(uint8_t* avg_power)
    \brief This function gets Average Power.
    \param avg_power - value of Average Power (binary encoded).
    \return The function call status, pass/fail.
*/
int pmic_get_average_soc_power(uint8_t *avg_power);

#endif
