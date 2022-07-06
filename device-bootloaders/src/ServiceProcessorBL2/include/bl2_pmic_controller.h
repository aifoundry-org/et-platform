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
/*! \file bl2_pmic_controller.h
    \brief A C header that defines the PMIC controller's public interfaces.
*/
/***********************************************************************/
#ifndef __BL2_PMIC_CONTROLLER_H__
#define __BL2_PMIC_CONTROLLER_H__

#include "dm_event_def.h"

#include "pmic_hal.h"

/*!
 * @enum  enum voltage_type_e
 * @brief Different voltage types supported by PMIC
 */
typedef enum
{
    DDR = 0,
    L2CACHE,
    MAXION,
    MINION,
    PCIE,
    NOC,
    PCIE_LOGIC,
    VDDQLP,
    VDDQ
} voltage_type_e;

/*!
 * @enum  enum pmb_value_type_e
 * @brief set value types supported by PMIC PMB Stats
 */
typedef enum
{
    V_OUT = 0,
    A_OUT,
    W_OUT,
    V_IN,
    A_IN,
    W_IN,
    DEG_C
} pmb_value_type_e;

/*!
 * @enum  enum pmb_module_type_e
 * @brief Different module types supported by PMIC
 */
typedef enum
{
    PMB_MINION = 0,
    PMB_NOC,
    PMB_SRAM,
} pmb_module_type_e;

/*!
 * @enum  enum pmb_reading_type_e
 * @brief Different reading types supported by PMIC
 */
typedef enum
{
    CURRENT = 0,
    MIN,
    MAX,
    AVERAGE,
} pmb_reading_type_e;

/*!
 * @struct struct pmic_event_control_block
 * @brief PMIC driver error mgmt control block
 */
typedef void (*dm_pmic_isr_callback)(uint8_t int_cause);
struct pmic_event_control_block
{
    uint32_t ce_count;                         /**< Correctable error count. */
    uint32_t uce_count;                        /**< Un-Correctable error count. */
    uint32_t ce_threshold;                     /**< Correctable error threshold. */
    dm_event_isr_callback event_cb;            /**< Event callback handler. */
    dm_pmic_isr_callback thermal_pwr_event_cb; /**< Thermal power event callback handler. */
};

#define PMIC_DDR_VOLTAGE_MULTIPLIER        5
#define PMIC_SRAM_VOLTAGE_MULTIPLIER       5
#define PMIC_MAXION_VOLTAGE_MULTIPLIER     5
#define PMIC_MINION_VOLTAGE_MULTIPLIER     5
#define PMIC_VDDQLP_VOLTAGE_MULTIPLIER     10
#define PMIC_VDDQ_VOLTAGE_MULTIPLIER       10
#define PMIC_PCIE_LOGIC_VOLTAGE_MULTIPLIER 625
#define PMIC_PCIE_VOLTAGE_MULTIPLIER       125

/* Macro to convert PMIC value to Hex */
#define PMIC_VOLTAGE_TO_HEX(val, idx)       (uint8_t)(((val - 250) / idx))
#define PMIC_HEX_TO_VOLTAGE(val, idx)       ((val * idx) + 250)
#define PMIC_PCIE_VOLTAGE_TO_HEX(val)       (uint8_t)(((val - 600) * 10 / 125))
#define PMIC_PCIE_LOGIC_VOLTAGE_TO_HEX(val) (uint8_t)(((val - 600) * 100 / 625))

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

/*! \fn int32_t pmic_thermal_pwr_cb_init(dm_pmic_isr_callback event_cb)
    \brief This function setup thermal power callback.
    \param event_cb - callback pointer
    \return The function call status, pass/fail.
*/
int32_t pmic_thermal_pwr_cb_init(dm_pmic_isr_callback event_cb);

/*! \fn void pmic_error_isr(void)
    \brief PMIC interrupt routine.
*/
void pmic_error_isr(void);

/*! \fn int pmic_get_fw_version(uint32_t* fw_version)
    \brief This function reads Firmware Version register of PMIC.
    \param fw_version - value of Firmware Version register of PMIC.
    \return The function call status, pass/fail.
*/
int pmic_get_fw_version(uint32_t *fw_version);

/*! \fn int pmic_get_gpio_bit(uint8_t index, uint8_t* gpio_bit_value)
    \brief This function reads GPIO Control register bit of PMIC.
    \param index - index of bit to be fetched
    \param gpio_bit_value - value of the bit
    \return The function call status, pass/fail.
*/
int pmic_get_gpio_bit(uint8_t index, uint8_t *gpoi_bit_value);

/*! \fn int pmic_set_gpio_bit(uint8_t index)
    \brief This function sets GPIO Control register bit of PMIC.
    \param index - index of bit to be set
    \return The function call status, pass/fail.
*/
int pmic_set_gpio_bit(uint8_t index);

/*! \fn int pmic_clear_gpo_bit(uint8_t index)
    \brief This function clears GPIO Control register bit of PMIC.
    \param index - index of bit to be cleared
    \return The function call status, pass/fail.
*/
int pmic_clear_gpio_bit(uint8_t index);

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

/*! \fn int pmic_read_instantenous_soc_power(uint8_t* soc_pwr)
    \brief This function returns soc input power.
    \param soc_pwr - value of Input Power (binary encoded).
    \return The function call status, pass/fail.
*/
int pmic_read_instantenous_soc_power(uint8_t *soc_pwr);

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

/*! \fn int pmic_get_reset_cause(uint32_t* reset_cause)
    \brief This function reads Reset Causation register of PMIC.
    \param reset_cause - value of Reset Causation register of PMIC.
    \return The function call status, pass/fail.
*/
int pmic_get_reset_cause(uint32_t *reset_cause);

/*! \fn int pmic_get_voltage(voltage_type_e voltage_type, uint8_t* voltage)
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
int pmic_get_voltage(voltage_type_e voltage_type, uint8_t *voltage);

/*! \fn int pmic_set_voltage(voltage_type_e voltage_type, uint8_t voltage)
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
*   - SRAM
    \param voltage - voltage value to be set (binary encoded)
    \return The function call status, pass/fail.
*/
int pmic_set_voltage(voltage_type_e voltage_type, uint8_t voltage);

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

/*! \fn int pmic_get_pmb_stats(pmb_component_type_e module_type, pmb_value_type_e value_type, uint8_t *current)
    \brief This function returns PMB stats os specified module.
    \param module_type - voltage type to be set:
*   - MINION
*   - NOC
*   - SRAM
    \param reading_type - reading type set
*   - CURRENT
*   - MIN
*   - MAX
*   - AVERAGE
*   \param value_type - value type set
*   - V_OUT
*   - A_OUT
*   - W_OUT
*   - V_IN
*   - A_IN
*   - W_IN
*   - DEG_C
    \param value - output current value in mA.
    \return The function call status, pass/fail.
*/
int32_t pmic_get_pmb_stats(pmb_module_type_e module_type, pmb_reading_type_e reading_type,
                           pmb_value_type_e value_type, uint32_t *value);

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

/*! \fn int pmic_get_wdog_timeout_time(uint32_t* wdog_time)
    \brief This function gets watchdog timeout time.
    \param wdog_time - wdog timeout time in miliseconds
    \return The function call status, pass/fail.
*/
int pmic_get_wdog_timeout_time(uint32_t *wdog_time);

/*! \fn int pmic_set_wdog_timeout_time(uint32_t timeout_time)
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

/*! \fn int pmic_set_tdp_threshold(uint8_t power_alarm)
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

/*! \fn int pmic_read_average_soc_power(uint8_t* avg_power)
    \brief This function gets Average Power.
    \param avg_power - value of Average Power (binary encoded).
    \return The function call status, pass/fail.
*/
int pmic_read_average_soc_power(uint8_t *avg_power);

/*! \fn int I2C_PMIC_Initialize(uint8_t i2c_id)
    \brief This function initializes PMIC
    \param i2c_id - I2C bus ID
    \return The function call status, pass/fail.
*/
int I2C_PMIC_Initialize(void);

/*! \fn int I2C_PMIC_Read (uint8_t reg)
    \brief This function reads data from PMIC register
    \param None
    \return value stored in register
*/
int I2C_PMIC_Read(uint8_t reg);

/*! \fn int I2C_PMIC_Write (uint8_t reg, uint8_t data)
    \brief This function writes data to PMIC register
    \param reg - register to write into
    \return The function call status, pass/fail
*/
int I2C_PMIC_Write(uint8_t reg, uint8_t data);

/*! \fn int pmic_get_int_config(uint8_t* int_config)
    \brief This function reads Interrupt Controller Configuration register of PMIC.
    \param int_config - interrupt configuration
    \return The function call status, pass/fail
*/
int pmic_get_int_config(uint8_t *int_config);

/*! \fn Power_Convert_Hex_to_mW(int8_t power_hex) 
    \brief This function converts PMIC encoded HEX value to real Power(mW)
    \param Hex  Power in PMIC encoded Hex value 
    \return Power in mW after conversion
*/
int32_t Power_Convert_Hex_to_mW(uint8_t power_hex);

/*! \fn Power_Convert_mW_to_Hex(int8_t power_mW) 
    \brief This function converts real Power(mW) to PMIC encoded HEX value
    \param Power in mW after conversion
    \return Hex  Power in PMIC encoded Hex value 
*/
int32_t Power_Convert_mW_to_Hex(uint8_t power_mW);

/*! \fn int pmic_get_board_type(uint32_t *board_type)
    \brief This function reads board type.
    \param board_type - board type BUB(0x1), PCIe(0x2)
    \return The function call status, pass/fail
*/
int pmic_get_board_type(uint32_t *board_type);

/*! \fn int pmic_set_gpio_as_output(uint8_t index)
    \brief This function sets GPIO direction as output.
    \param index - index of GPIO pin
    \return The function call status, pass/fail
*/
int pmic_set_gpio_as_output(uint8_t index);

/*! \fn int pmic_set_gpio_as_input(uint8_t index)
    \brief This function sets GPIO direction as input.
    \param index - index of GPIO pin
    \return The function call status, pass/fail
*/
int pmic_set_gpio_as_input(uint8_t index);

#endif
