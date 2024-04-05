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

#include "dm.h"
#include "dm_event_def.h"

#include "pmic_hal.h"

/*!
 * @enum  enum pmb_module_type_e
 * @brief Different module types supported by PMIC
 */
typedef enum
{
    PMB_MINION = 0,
    PMB_NOC,
    PMB_SRAM,
    PMB_MAX_MODULE_TYPE
} pmb_module_type_e;

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
    DEG_C,
    PMB_MAX_VALUE_TYPE
} pmb_value_type_e;

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
    PMB_MAX_READING_TYPE
} pmb_reading_type_e;

/*! \struct pmb_reading_type_t
    \brief Struct use to hold PMB stat reading type
*/
struct pmb_reading_type_t
{
    uint16_t current; /**<current value*/
    uint16_t min;     /**< minimum value */
    uint16_t max;     /**< maximum value */
    uint16_t average; /**< average value */
} __attribute__((packed));

/*! \struct pmb_values_t
    \brief Struct use to hold PMB stat values
*/
struct pmb_values_t
{
    struct pmb_reading_type_t v_out; /**< Voltage output*/
    struct pmb_reading_type_t a_out; /**< Ampere output */
    struct pmb_reading_type_t w_out; /**< Power output */
    struct pmb_reading_type_t v_in;  /**< Voltage input */
    struct pmb_reading_type_t a_in;  /**< Ampere input */
    struct pmb_reading_type_t w_in;  /**< Power input */
    struct pmb_reading_type_t deg_c; /**< Temperature */
} __attribute__((packed));

/*! \struct pmb_stats_t
    \brief Struct use to hold PMB stats for minion, noc and sram
*/
struct pmb_stats_t
{
    struct pmb_values_t minion; /**< Minion stats */
    struct pmb_values_t noc;    /**< NOC stats */
    struct pmb_values_t sram;   /**< SRAM stats */
} __attribute__((packed));

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

/*! \struct imageMetadata_
    \brief Struct use to image metadata
    \warning Keep it synced with PMIC FW.
*/
typedef struct imageMetadata_
{
    uint32_t start_addr;
    uint32_t version;
    uint32_t supported_board_types;
    char hash[16];
    uint32_t checksum;
    uint32_t image_size;
    uint32_t bl_fw_version;
    uint32_t sp_pmic_interface_version;
    uint32_t metadata_version;
    uint32_t build_type;
} imageMetadata_t;

/*! \struct imageMetadata_
    \brief Macro for PMIC FW image
*/
#define PMIC_FW_IMAGE_METADATA_OFFSET 0x100

/* Base for each voltage rail */
#define PMIC_DDR_VOLTAGE_BASE        250
#define PMIC_SRAM_VOLTAGE_BASE       250
#define PMIC_MAXION_VOLTAGE_BASE     250
#define PMIC_MINION_VOLTAGE_BASE     250
#define PMIC_NOC_VOLTAGE_BASE        250
#define PMIC_VDDQLP_VOLTAGE_BASE     250
#define PMIC_VDDQ_VOLTAGE_BASE       250
#define PMIC_PCIE_LOGIC_VOLTAGE_BASE 600
#define PMIC_PCIE_VOLTAGE_BASE       600

/* Multiplier for each voltage rail */
#define PMIC_DDR_VOLTAGE_MULTIPLIER        5
#define PMIC_SRAM_VOLTAGE_MULTIPLIER       5
#define PMIC_MAXION_VOLTAGE_MULTIPLIER     5
#define PMIC_MINION_VOLTAGE_MULTIPLIER     5
#define PMIC_NOC_VOLTAGE_MULTIPLIER        5
#define PMIC_VDDQLP_VOLTAGE_MULTIPLIER     10
#define PMIC_VDDQ_VOLTAGE_MULTIPLIER       10
#define PMIC_PCIE_LOGIC_VOLTAGE_MULTIPLIER 625
#define PMIC_PCIE_VOLTAGE_MULTIPLIER       125

/* Divider for each voltage rail */
#define PMIC_GENERIC_VOLTAGE_DIVIDER    1
#define PMIC_PCIE_LOGIC_VOLTAGE_DIVIDER 100
#define PMIC_PCIE_VOLTAGE_DIVIDER       10

/* Macro to encode voltage value to 8 bit encoding */
#define PMIC_MILLIVOLT_TO_HEX(mV_val, base, multiplier, divider) \
    (uint8_t)(((mV_val - base) * divider) / multiplier)
#define PMIC_HEX_TO_MILLIVOLT(hex_val, base, multiplier, divider) \
    (((hex_val * multiplier) / divider) + base)

/*! \def EXTRACT_BYTE(byte_idx, org_val)
    \brief Macro definition to extract the specified byte from a variable.
*/
#define EXTRACT_BYTE(byte_idx, org_val) (0xFF & (org_val >> (byte_idx * 8)))

/*! \def PMIC_BUSY_WAIT_TIMEOUT_MS
    \brief Macro definition for PMIC wait timeout when busy flag is set
*/
#define PMIC_BUSY_WAIT_TIMEOUT_MS 500

/*! \def POWER_THRESHOLD_HW_CATASTROPHIC
    \brief A macro that provides power threshold value.
*          This power threshold will be written to PMIC and PMIC will be expected
*          to raise the interrupt if power goes beyond this threshold.
*          Following M.2 spec.
*/
#define POWER_THRESHOLD_HW_CATASTROPHIC 75

/*! \def TEMP_THRESHOLD_HW_CATASTROPHIC
    \brief A macro that provides pmic temperature threshold value
*/
#define TEMP_THRESHOLD_HW_CATASTROPHIC 75

/*! \def PMIC_TEMP_LOWER_SET_LIMIT
    \brief A macro that provides pmic minimum temperature threshold value
*/
#define PMIC_TEMP_LOWER_SET_LIMIT 55

/*! \def PMIC_TEMP_UPPER_SET_LIMIT
    \brief A macro that provides pmic maximum temperature threshold value
*/
#define PMIC_TEMP_UPPER_SET_LIMIT 75

/*! \fn void setup_pmic(void)
    \brief This function initialize I2C connection.
*/
void setup_pmic(void);

/*! \fn bool pmic_check_firmware_updated(void)
    \brief This function returns true if PMIC update was done.
    \return true or false
*/
bool pmic_check_firmware_updated(void);

/*! \fn int wait_pmic_ready(void)
    \brief This function waits for PMIC ready to assert or time out.
    \return The function call status, pass/fail.
*/
int wait_pmic_ready(void);

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

/*! \fn int pmic_get_fw_src_hash(uint32_t* fw_src_hash)
    \brief This function reads Firmware Git Hash of PMIC source.
    \param fw_src_hash - GIT hash value of Firmware source.
    \return The function call status, pass/fail.
*/
int pmic_get_fw_src_hash(uint32_t *fw_src_hash);

/*! \fn int pmic_get_fw_version(uint8_t *major, uint8_t *minor, uint8_t *patch)
    \brief This function retrieves the currect PMIC FW version.
    \return The function call status, pass/fail.
*/
int32_t pmic_get_fw_version(uint8_t *major, uint8_t *minor, uint8_t *patch);

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

/*! \fn int pmic_read_instantaneous_soc_power(uint16_t *soc_pwr_10mW)
    \brief This function returns soc input power in 10 mW steps.
    \param soc_pwr_10mW Value of Input Power (10 mW steps).
    \return The function call status, pass/fail.
*/
int pmic_read_instantaneous_soc_power(uint16_t *soc_pwr_10mW);

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

/*! \fn int pmic_get_voltage(module_e voltage_type, uint8_t* voltage)
    \brief This function returns specific voltage setting.
    \param voltage_type - voltage type to be read:
*   - MODULE_DDR
*   - MODULE_L2CACHE
*   - MODULE_MAXION
*   - MODULE_MINION
*   - MODULE_PCIE
*   - MODULE_NOC
*   - MODULE_PCIE_LOGIC
*   - MODULE_VDDQLP
*   - MODULE_VDDQ
    \param voltage - voltage value (binary encoded)
    \return The function call status, pass/fail.
*/
int pmic_get_voltage(module_e voltage_type, uint8_t *voltage);

/*! \fn int pmic_set_voltage(module_e voltage_type, uint8_t voltage)
    \brief This function returns specific voltage setting.
    \param voltage_type - voltage type to be set:
*   - MODULE_DDR
*   - MODULE_L2CACHE
*   - MODULE_MAXION
*   - MODULE_MINION
*   - MODULE_PCIE
*   - MODULE_NOC
*   - MODULE_PCIE_LOGIC
*   - MODULE_VDDQLP
*   - MODULE_VDDQ
*   - MODULE_SRAM
    \param voltage - voltage value to be set (binary encoded)
    \return The function call status, pass/fail.
*/
int pmic_set_voltage(module_e voltage_type, uint8_t voltage);

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

/*! \fn int pmic_reset_pmb_stats(void)
    \brief This function resets PMB stats.
    \return The function call status, pass/fail.
*/
int pmic_reset_pmb_stats(void);

/*! \fn int pmic_get_pmb_stats(struct pmb_stats_t *pmb_stats)
    \brief This function returns PMB stats os specified module.
    \param pmb_stats - pmb stats structure to hold stat values for:
*   - MINION
*   - NOC
*   - SRAM
    \return The function call status, pass/fail.
*/
int32_t pmic_get_pmb_stats(struct pmb_stats_t *pmb_stats);

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

/*! \fn int pmic_set_tdp_threshold(uint16_t power_alarm)
    \brief This function sets power alarm point.
    \param power_alarm - power alarm point to be set (binary encoded)
    \return The function call status, pass/fail.
*/
int pmic_set_tdp_threshold(uint16_t power_alarm);

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

/*! \fn int pmic_read_average_soc_power(uint16_t *avg_pwr_10mw)
    \brief This function gets Average Power in 10 mW steps.
    \param avg_pwr_10mw Value of Average Power (10 mW steps).
    \return The function call status, pass/fail.
*/
int pmic_read_average_soc_power(uint16_t *avg_pwr_10mw);

/*! \fn int pmic_read_fru(uint8_t *fru_data, uint32_t size)
    \brief This function gets the FRU data from the NVM.
    \param fru_data Value of FRU data. 
    \param size     Size of FRU data. 
    \return The function call status, pass/fail.
*/
int pmic_read_fru(uint8_t *fru_data, uint32_t size);

/*! \fn int pmic_set_fru(const uint8_t *fru_data, uint32_t size)
    \brief This function sets the FRU data to the NVM.
    \param fru_data Value of FRU data. 
    \param size     Size of FRU data. 
    \return The function call status, pass/fail.
*/
int pmic_set_fru(const uint8_t *fru_data, uint32_t size);

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

/*! \fn int pmic_get_board_type(uint8_t *board_type, uint8_t* board_design_revision, uint8_t* board_modification_revision, uint8_t* board_id)
    \brief This function reads board version information.
    \param board_type - board type BUB(0x1), PCIe(0x2)
    \param board_design_revision - board design revision
    \param board_modification_revision - board modification revision
    \param board_id - board unique id set at producrion
    \return The function call status, pass/fail
*/
int pmic_get_board_type(uint8_t *board_type, uint8_t *board_design_revision,
                        uint8_t *board_modification_revision, uint8_t *board_id);

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

/*! \fn int pmic_firmware_update(void)
    \brief This function updates the inactive pmic boot slot with the firmware
    \return The function call status, pass/fail
*/
int pmic_firmware_update(void);

#endif
