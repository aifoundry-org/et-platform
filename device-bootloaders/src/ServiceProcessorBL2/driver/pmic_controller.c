/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
************************************************************************/
/*! \file pmic_controller.c
    \brief A C module that implements the PMIC controller's functionality. It
    provides functions to set/get voltage of different components and set/clear
    gpio bits. It also configures different error thresholds and report error
    events to host.

    Public interfaces:
        setup_pmic
        pmic_error_control_init
        pmic_error_isr
        pmic_get_fw_version
        pmic_get_gpio_bit
        pmic_set_gpio_bit
        pmic_clear_gpio_bit
        pmic_get_input_voltage
        pmic_get_temperature_threshold
        pmic_set_temperature_threshold
        pmic_get_temperature
        pmic_read_instantaneous_soc_power
        pmic_enable_etsoc_reset_after_perst
        pmic_disable_etsoc_reset_after_perst
        pmic_get_reset_cause
        pmic_get_voltage
        pmic_set_voltage
        pmic_get_minion_group_voltage
        pmic_set_minion_group_voltage
        pmic_enable_wdog_timer
        pmic_disable_wdog_timer
        pmic_enable_wdog_timeout_reset
        pmic_disable_wdog_timeout_reset
        pmic_get_wdog_timeout_time
        pmic_set_wdog_timeout_time
        pmic_get_tdp_threshold
        pmic_set_tdp_threshold
        pmic_force_shutdown
        pmic_force_power_off_on
        pmic_force_reset
        pmic_force_perst
        pmic_reset_wdog_timer
        pmic_read_average_soc_power
        pmic_get_pmb_stats
        I2C_PMIC_Initialize
        I2C_PMIC_Read
        I2C_PMIC_Write
*/
/***********************************************************************/

/**
* @file $Id$
* @version $Release$
* @date $Date$
* @author
*
* @brief bl2_pmic_controller.c Main controller to the external PMIC
*
*/
/*======================================================================== */

#include <stdint.h>
#include <stdio.h>
#include "etsoc/isa/io.h"
#include "bl2_i2c_driver.h"
#include "bl2_gpio_controller.h"
#include "bl2_pmic_controller.h"
#include "bl2_main.h"
#include "interrupt.h"
#include "bl_error_code.h"
#include "log.h"
#include "delays.h"

#include "hwinc/hal_device.h"
#include "hwinc/sp_cru_reset.h"

#define PMIC_SLAVE_ADDRESS         0x42
#define PMIC_GPIO_INT_PIN_NUMBER   0x1
#define ENABLE_ALL_PMIC_INTERRUPTS 0xFF
#define BYTE_SIZE                  8

/* PMB related defines */
#define PMB_READ_BYTES 4
#define PMB_STATS_MASK 0xffff

static struct pmic_event_control_block event_control_block __attribute__((section(".data")));

/* Generic PMIC setup */
static ET_I2C_DEV_t g_pmic_i2c_dev_reg;

/************************************************************************
*
*   FUNCTION
*
*       set_pmic_i2c_dev
*
*   DESCRIPTION
*
*       This function initialize I2C registers pointer.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

static void set_pmic_i2c_dev(void)
{
    g_pmic_i2c_dev_reg.regs = (I2c *)R_SP_I2C0_BASEADDR;
    g_pmic_i2c_dev_reg.isInitialized = false;
}

/************************************************************************
*
*   FUNCTION
*
*       I2C_PMIC_Initialize
*
*   DESCRIPTION
*
*       This function initializes PMIC.
*
*   INPUTS
*
*       i2c_id      i2C bus ID
*
*   OUTPUTS
*
*       status     status of initialization success/error
*
***********************************************************************/
int I2C_PMIC_Initialize(void)
{
    setup_pmic();
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_pmic_reg
*
*   DESCRIPTION
*
*       This function reads PMIC register via I2C.
*
*   INPUTS
*
*       reg        register address
*       reg_size   register size in bytes
*
*   OUTPUTS
*
*       reg_value  value of register
*
***********************************************************************/

inline static int get_pmic_reg(uint8_t reg, uint8_t *reg_value, uint8_t reg_size)
{
    if (0 != i2c_read(&g_pmic_i2c_dev_reg, reg, reg_value, reg_size))
    {
        MESSAGE_ERROR("PMIC read failed!");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       set_pmic_reg
*
*   DESCRIPTION
*
*       This function writes PMIC register via I2C.
*
*   INPUTS
*
*       reg        register address
*       value      value to be written
*       reg_size   register size in bytes
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

inline static int set_pmic_reg(uint8_t reg, uint8_t value, uint8_t reg_size)
{
    if (0 != i2c_write(&g_pmic_i2c_dev_reg, reg, &value, reg_size))
    {
        MESSAGE_ERROR("PMIC write failed!");
        return ERROR_PMIC_I2C_WRITE_FAILED;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_pmb_rstat
*
*   DESCRIPTION
*
*       This function gets PMB status for a particular value type. Value types
*       supported are v_out, a_out, w_out, v_in, a_in, w_in, deg_c.
*
*   INPUTS
*
*       stat_reading        pmb stats reading placeholder
*
*   OUTPUTS
*
*       SUCCESS or any error value
*
***********************************************************************/
static int pmic_get_pmb_rstat(struct pmb_reading_type_t *stat_reading)
{
    int status = STATUS_SUCCESS;
    uint32_t temp_val = 0;

    status = get_pmic_reg(PMIC_I2C_PMB_RW_ADDRESS, (uint8_t *)&temp_val, PMB_READ_BYTES);
    if (status == STATUS_SUCCESS)
    {
        stat_reading->current = (temp_val & PMB_STATS_MASK);
        status = get_pmic_reg(PMIC_I2C_PMB_RW_ADDRESS, (uint8_t *)&temp_val, PMB_READ_BYTES);
    }
    if (status == STATUS_SUCCESS)
    {
        stat_reading->min = (temp_val & PMB_STATS_MASK);
        status = get_pmic_reg(PMIC_I2C_PMB_RW_ADDRESS, (uint8_t *)&temp_val, PMB_READ_BYTES);
    }
    if (status == STATUS_SUCCESS)
    {
        stat_reading->max = (temp_val & PMB_STATS_MASK);
        status = get_pmic_reg(PMIC_I2C_PMB_RW_ADDRESS, (uint8_t *)&temp_val, PMB_READ_BYTES);
    }
    if (status == STATUS_SUCCESS)
    {
        stat_reading->average = (temp_val & PMB_STATS_MASK);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_int_config
*
*   DESCRIPTION
*
*       This function reads Interrupt Controller Configuration
*       register of PMIC.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       int_config   value of Interrupt Controller Configuration register of PMIC.
*
***********************************************************************/

int pmic_get_int_config(uint8_t *int_config)
{
    return (get_pmic_reg(PMIC_I2C_INT_CTRL_ADDRESS, int_config, 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_set_int_config
*
*   DESCRIPTION
*
*       This function sets Interrupt Controller Configuration
*       register of PMIC.
*
*   INPUTS
*
*       int_cfg            value to be set
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

static int pmic_set_int_config(uint8_t int_cfg)
{
    return set_pmic_reg(PMIC_I2C_INT_CTRL_ADDRESS, int_cfg, 1);
}

/************************************************************************
*
*   FUNCTION
*
*       setup_pmic
*
*   DESCRIPTION
*
*       This function initialize I2C connection.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

void setup_pmic(void)
{
    set_pmic_i2c_dev();

    if (0 != i2c_init(&g_pmic_i2c_dev_reg, ET_I2C_SPEED_400k, PMIC_SLAVE_ADDRESS))
    {
        MESSAGE_ERROR("PMIC connection failed to establish link\n");
    }

    /* Set temperature threshold values */
    pmic_set_temperature_threshold(TEMP_THRESHOLD_HW_CATASTROPHIC);

    /* Set power threshold values */
    pmic_set_tdp_threshold(POWER_THRESHOLD_HW_CATASTROPHIC << 2);

    /* Enable all PMIC interrupts */
    if (0 != pmic_set_int_config(ENABLE_ALL_PMIC_INTERRUPTS))
    {
        MESSAGE_ERROR("Failed to enable PMIC interrupts!");
    }

    /* Configure and enable GPIO interrupt */
    if (0 != gpio_config_interrupt(GPIO_CONTROLLER_ID_SPIO, PMIC_GPIO_INT_PIN_NUMBER, GPIO_INT_EDGE,
                                   GPIO_INT_LOW, GPIO_INT_DEBOUNCE_OFF))
    {
        MESSAGE_ERROR("Failed to configure GPIO PMIC interrupt!");
    }

    if (0 != gpio_enable_interrupt(GPIO_CONTROLLER_ID_SPIO, PMIC_GPIO_INT_PIN_NUMBER))
    {
        MESSAGE_ERROR("Failed to enable GPIO PMIC interrupt!");
    }

    INT_enableInterrupt(SPIO_PLIC_GPIO_INTR, 1, pmic_error_isr);

    Log_Write(LOG_LEVEL_INFO, "PMIC connection establish\n");
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_error_control_init
*
*   DESCRIPTION
*
*       This function setup error callback.
*
*   INPUTS
*
*       event_cb    callback pointer
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int32_t pmic_error_control_init(dm_event_isr_callback event_cb)
{
    event_control_block.event_cb = event_cb;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_power_cb_init
*
*   DESCRIPTION
*
*       This function setup thermal power callback.
*
*   INPUTS
*
*       event_cb    callback pointer
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int32_t pmic_thermal_pwr_cb_init(dm_pmic_isr_callback event_cb)
{
    event_control_block.thermal_pwr_event_cb = event_cb;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_int_cause
*
*   DESCRIPTION
*
*       This function reads Interrupt Controller Causation
*       register of PMIC.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       int_cause    value of Interrupt Controller Causation register of PMIC.
*
***********************************************************************/

static int pmic_get_int_cause(uint8_t *int_cause)
{
    return (get_pmic_reg(PMIC_I2C_INT_CAUSE_ADDRESS, int_cause, 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_reg_fault_details
*
*   DESCRIPTION
*
*       This function reads Regulator Fault Details
*       register of PMIC.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       reg_fault_details    value of Regulator Fault Details register
*                            of PMIC.
*
***********************************************************************/

static int pmic_get_reg_fault_details(uint32_t *reg_fault_details)
{
    return (get_pmic_reg(PMIC_I2C_REG_FAULT_REG_FAULT_ADDRESS, (uint8_t *)reg_fault_details, 4));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_reg_comm_fault_details
*
*   DESCRIPTION
*
*       This function reads Regulator Comm Fail Details
*       register of PMIC.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       reg_comm_fault_details    value of Regulator Comm Fail Details
*                                 register of PMIC.
*
***********************************************************************/

static int pmic_get_reg_comm_fault_details(uint32_t *reg_comm_fault_details)
{
    return (
        get_pmic_reg(PMIC_I2C_REG_COM_FAIL_DETAILS_ADDRESS, (uint8_t *)reg_comm_fault_details, 4));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_command_comm_fault_details
*
*   DESCRIPTION
*
*       This function reads Command Comm Fail Details
*       register of PMIC.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       command_comm_fault_details    value of Command Comm Fail Details
*                                     register of PMIC.
*
***********************************************************************/

static int pmic_get_command_comm_fault_details(uint32_t *command_comm_fault_details)
{
    return (get_pmic_reg(PMIC_I2C_COMMAND_COMM_FAIL_DETAILS_ADDRESS,
                         (uint8_t *)command_comm_fault_details, 4));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_error_isr
*
*   DESCRIPTION
*
*       PMIC interrupt routine.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

void pmic_error_isr(void)
{
    uint8_t int_cause = 0;
    struct event_message_t message;
    uint8_t reg_value = 0;
    uint16_t reg_value_16 = 0;
    uint32_t reg_value_32 = 0;

    if (0 != gpio_clear_interrupt(GPIO_CONTROLLER_ID_SPIO, PMIC_GPIO_INT_PIN_NUMBER))
    {
        MESSAGE_ERROR("GPIO int clear failed!");
    }

    if (0 != pmic_get_int_cause(&int_cause))
    {
        MESSAGE_ERROR("PMIC read failed!");
    }

    /* Generate PMIC Error */

    if (PMIC_I2C_INT_CAUSE_OV_TEMP_GET(int_cause))
    {
        event_control_block.thermal_pwr_event_cb(int_cause);

        if (0 != pmic_get_temperature(&reg_value))
        {
            MESSAGE_ERROR("PMIC read failed!");
        }
        FILL_EVENT_HEADER(&message.header, PMIC_ERROR, sizeof(struct event_message_t))
        FILL_EVENT_PAYLOAD(&message.payload, FATAL, 0, 1 << PMIC_I2C_INT_CAUSE_OV_TEMP_LSB,
                           reg_value)
        event_control_block.event_cb(UNCORRECTABLE, &message);
    }

    if (PMIC_I2C_INT_CAUSE_OV_POWER_GET(int_cause))
    {
        event_control_block.thermal_pwr_event_cb(int_cause);

        if (0 != pmic_read_instantaneous_soc_power(&reg_value_16))
        {
            MESSAGE_ERROR("PMIC read failed!");
        }
        FILL_EVENT_HEADER(&message.header, PMIC_ERROR, sizeof(struct event_message_t))
        FILL_EVENT_PAYLOAD(&message.payload, FATAL, 0, 1 << PMIC_I2C_INT_CAUSE_OV_POWER_LSB,
                           reg_value_16)
        event_control_block.event_cb(UNCORRECTABLE, &message);
    }

    if (PMIC_I2C_INT_CAUSE_PWR_FAIL_GET(int_cause))
    {
        if (0 != pmic_get_input_voltage(&reg_value))
        {
            MESSAGE_ERROR("PMIC read failed!");
        }
        FILL_EVENT_HEADER(&message.header, PMIC_ERROR, sizeof(struct event_message_t))
        FILL_EVENT_PAYLOAD(&message.payload, FATAL, 0, 1 << PMIC_I2C_INT_CAUSE_PWR_FAIL_LSB,
                           reg_value)
        event_control_block.event_cb(UNCORRECTABLE, &message);
    }

    if (PMIC_I2C_INT_CAUSE_MINION_DROOP_GET(int_cause))
    {
        FILL_EVENT_HEADER(&message.header, PMIC_ERROR, sizeof(struct event_message_t))
        FILL_EVENT_PAYLOAD(&message.payload, FATAL, 0, 1 << PMIC_I2C_INT_CAUSE_MINION_DROOP_LSB, 0)
        event_control_block.event_cb(UNCORRECTABLE, &message);
    }

    if (PMIC_I2C_INT_CAUSE_MESSAGE_ERROR_GET(int_cause))
    {
        if (0 != pmic_get_command_comm_fault_details(&reg_value_32))
        {
            MESSAGE_ERROR("PMIC read failed!");
        }
        FILL_EVENT_HEADER(&message.header, PMIC_ERROR, sizeof(struct event_message_t))
        FILL_EVENT_PAYLOAD(&message.payload, FATAL, 0, 1 << PMIC_I2C_INT_CAUSE_MESSAGE_ERROR_LSB,
                           reg_value_32)
        event_control_block.event_cb(UNCORRECTABLE, &message);
    }

    if (PMIC_I2C_INT_CAUSE_REG_COM_FAIL_GET(int_cause))
    {
        if (0 != pmic_get_reg_comm_fault_details(&reg_value_32))
        {
            MESSAGE_ERROR("PMIC read failed!");
        }
        FILL_EVENT_HEADER(&message.header, PMIC_ERROR, sizeof(struct event_message_t))
        FILL_EVENT_PAYLOAD(&message.payload, FATAL, 0, 1 << PMIC_I2C_INT_CAUSE_REG_COM_FAIL_LSB,
                           reg_value_32)
        event_control_block.event_cb(UNCORRECTABLE, &message);
    }

    if (PMIC_I2C_INT_CAUSE_REG_FAULT_GET(int_cause))
    {
        if (0 != pmic_get_reg_fault_details(&reg_value_32))
        {
            MESSAGE_ERROR("PMIC read failed!");
        }
        FILL_EVENT_HEADER(&message.header, PMIC_ERROR, sizeof(struct event_message_t))
        FILL_EVENT_PAYLOAD(&message.payload, FATAL, 0, 1 << PMIC_I2C_INT_CAUSE_REG_FAULT_LSB,
                           reg_value_32)
        event_control_block.event_cb(UNCORRECTABLE, &message);
    }
}

/* Specific Register Access */

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_fw_version
*
*   DESCRIPTION
*
*       This function reads Firmware Version register of PMIC.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       fw_version    value of Firmware Version register of PMIC.
*
***********************************************************************/

int pmic_get_fw_version(uint32_t *fw_version)
{
    return (get_pmic_reg(PMIC_I2C_FIRMWARE_VERSION_ADDRESS, (uint8_t *)fw_version, 4));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_board_type
*
*   DESCRIPTION
*
*       This function reads Board Type register of PMIC.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       board_type    value of Board Type (BUB, PCIe) register of PMIC.
*
***********************************************************************/

int pmic_get_board_type(uint32_t *board_type)
{
    return (get_pmic_reg(PMIC_I2C_BOARD_TYPE_ADDRESS, (uint8_t *)board_type, 4));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_set_gpio_as_input
*
*   DESCRIPTION
*
*       This function sets GPIO Config register bit of PMIC.
*
*   INPUTS
*
*       index   index of bit to set direction
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_set_gpio_as_input(uint8_t index)
{
    uint8_t reg_value;

    if (index > 7)
    {
        MESSAGE_ERROR("Index out of range!");
        return ERROR_PMIC_I2C_INVALID_ARGUMENTS;
    }

    if (0 != get_pmic_reg(PMIC_I2C_GPIO_CONFIG_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    reg_value = reg_value & (uint8_t)(~(0x1u << index));

    return (set_pmic_reg(PMIC_I2C_GPIO_CONFIG_ADDRESS,
                         PMIC_I2C_GPIO_CONFIG_INPUT_OUTPUT_SET(reg_value), 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_set_gpio_as_output
*
*   DESCRIPTION
*
*       This function sets GPIO Config register bit of PMIC.
*
*   INPUTS
*
*       index   index of bit to set direction
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_set_gpio_as_output(uint8_t index)
{
    uint8_t reg_value;

    if (index > 7)
    {
        MESSAGE_ERROR("Index out of range!");
        return ERROR_PMIC_I2C_INVALID_ARGUMENTS;
    }

    if (0 != get_pmic_reg(PMIC_I2C_GPIO_CONFIG_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    reg_value = reg_value | (uint8_t)(0x1u << index);

    return (set_pmic_reg(PMIC_I2C_GPIO_CONFIG_ADDRESS,
                         PMIC_I2C_GPIO_CONFIG_INPUT_OUTPUT_SET(reg_value), 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_gpio_bit
*
*   DESCRIPTION
*
*       This function reads GPIO RW register bit of PMIC.
*
*   INPUTS
*
*       index   index of bit to be fetched
*
*   OUTPUTS
*
*       gpio_bit_value   value of GPIO RW register bit of PMIC.
*
***********************************************************************/

int pmic_get_gpio_bit(uint8_t index, uint8_t *gpio_bit_value)
{
    uint8_t reg_value;

    if (index > 7)
    {
        MESSAGE_ERROR("Index out of range!");
        return ERROR_PMIC_I2C_INVALID_ARGUMENTS;
    }

    if (0 != get_pmic_reg(PMIC_I2C_GPIO_RW_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    *gpio_bit_value = ((uint8_t)(reg_value >> index) & 0x1u);

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_set_gpio_bit
*
*   DESCRIPTION
*
*       This function sets GPIO RW register bit of PMIC.
*
*   INPUTS
*
*       index   index of bit to be set
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_set_gpio_bit(uint8_t index)
{
    uint8_t reg_value;

    if (index > 7)
    {
        MESSAGE_ERROR("Index out of range!");
        return ERROR_PMIC_I2C_INVALID_ARGUMENTS;
    }

    if (0 != get_pmic_reg(PMIC_I2C_GPIO_RW_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    reg_value = reg_value | (uint8_t)(0x1u << index);

    return (
        set_pmic_reg(PMIC_I2C_GPIO_RW_ADDRESS, PMIC_I2C_GPIO_RW_INPUT_OUTPUT_SET(reg_value), 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_clear_gpio_bit
*
*   DESCRIPTION
*
*       This function clears GPIO RW register bit of PMIC.
*
*   INPUTS
*
*       index   index of bit to be cleared
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_clear_gpio_bit(uint8_t index)
{
    uint8_t reg_value;

    if (index > 7)
    {
        MESSAGE_ERROR("Index out of range!");
        return ERROR_PMIC_I2C_INVALID_ARGUMENTS;
    }

    if (0 != get_pmic_reg(PMIC_I2C_GPIO_RW_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    reg_value = reg_value & (uint8_t)(~(0x1u << index));

    return (
        set_pmic_reg(PMIC_I2C_GPIO_RW_ADDRESS, PMIC_I2C_GPIO_RW_INPUT_OUTPUT_SET(reg_value), 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_input_voltage
*
*   DESCRIPTION
*
*       This function reads Input Voltage register of PMIC.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       input_voltage   value of Input Voltage register of PMIC in mV.
*
***********************************************************************/

int pmic_get_input_voltage(uint8_t *input_voltage)
{
    return (get_pmic_reg(PMIC_I2C_INPUT_VOLTAGE_ADDRESS, input_voltage, 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_temperature_threshold
*
*   DESCRIPTION
*
*       This function reads Temperature Alarm Configuration register of PMIC.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       temp_threshold    value of Temperature Alarm Configuration register of PMIC.
*
***********************************************************************/

int pmic_get_temperature_threshold(uint8_t *temp_threshold)
{
    return (get_pmic_reg(PMIC_I2C_TEMP_ALARM_CONF_ADDRESS, temp_threshold, 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_set_temperature_threshold
*
*   DESCRIPTION
*
*       This function sets Temperature Alarm Configuration
*       register of PMIC.
*
*   INPUTS
*
*       temp_limit        value to be set
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_set_temperature_threshold(uint8_t temp_limit)
{
    if ((temp_limit < PMIC_TEMP_LOWER_SET_LIMIT) || (temp_limit > PMIC_TEMP_UPPER_SET_LIMIT))
    {
        MESSAGE_ERROR("Error unsupported Temperature limits\n");
        return ERROR_PMIC_I2C_INVALID_ARGUMENTS;
    }
    else
    {
        return set_pmic_reg(PMIC_I2C_TEMP_ALARM_CONF_ADDRESS, temp_limit, 1);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_temperature
*
*   DESCRIPTION
*
*       This function reads System Temperature register of PMIC.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       sys_temp    value of System Temperature register of PMIC.
*
***********************************************************************/

int pmic_get_temperature(uint8_t *sys_temp)
{
    return (get_pmic_reg(PMIC_I2C_SYSTEM_TEMP_ADDRESS, sys_temp, 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_read_instantaneous_soc_power
*
*   DESCRIPTION
*
*       This function returns soc input power in mW.
*
*   INPUTS
*
*       uint8_t  Pointer to data to load result to
*
*   OUTPUTS
*
*       soc_pwr    value of Input Power in binary encoded value.
*
***********************************************************************/

int pmic_read_instantaneous_soc_power(uint16_t *soc_pwr)
{
    int status;

    /* read input power from PMIC */
    status = get_pmic_reg(PMIC_I2C_INPUT_POWER_ADDRESS, (uint8_t *)soc_pwr, 2);
    if (status == STATUS_SUCCESS)
    {
        /* Power is in 10 mW units as defined in PMIC docs*/
        *soc_pwr = (uint16_t)((*soc_pwr) * 10);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_enable_etsoc_reset_after_perst
*
*   DESCRIPTION
*
*       This function enables a PERST event to also reset the ET-SOC.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_enable_etsoc_reset_after_perst(void)
{
    uint8_t reg_value;

    if (0 != get_pmic_reg(PMIC_I2C_RESET_CTRL_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    return (set_pmic_reg(PMIC_I2C_RESET_CTRL_ADDRESS,
                         PMIC_I2C_RESET_CTRL_PERST_EN_MODIFY(reg_value, 1), 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_disable_etsoc_reset_after_perst
*
*   DESCRIPTION
*
*       This function disables a PERST event to also reset the ET-SOC.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_disable_etsoc_reset_after_perst(void)
{
    uint8_t reg_value;

    if (0 != get_pmic_reg(PMIC_I2C_RESET_CTRL_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    return (set_pmic_reg(PMIC_I2C_RESET_CTRL_ADDRESS,
                         PMIC_I2C_RESET_CTRL_PERST_EN_MODIFY(reg_value, 0), 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_reset_cause
*
*   DESCRIPTION
*
*       This function reads Reset Causation register of PMIC.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       reset_cause    value of Reset Causation register of PMIC.
*
***********************************************************************/

int pmic_get_reset_cause(uint32_t *reset_cause)
{
    return (get_pmic_reg(PMIC_I2C_RESET_RESET_CAUSE_ADDRESS, (uint8_t *)reset_cause, 4));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_voltage
*
*   DESCRIPTION
*
*       This function returns specific voltage setting.
*
*   INPUTS
*
*       voltage_type      voltage type to be read
*
*   OUTPUTS
*
*       voltage           voltage value in mV
*
***********************************************************************/

int pmic_get_voltage(module_e voltage_type, uint8_t *voltage)
{
    switch (voltage_type)
    {
        case MODULE_DDR:
            return (get_pmic_reg(PMIC_I2C_DDR_VOLTAGE_ADDRESS, voltage, 1));
        case MODULE_L2CACHE:
            return (get_pmic_reg(PMIC_I2C_L2_CACHE_VOLTAGE_ADDRESS, voltage, 1));
        case MODULE_MAXION:
            return (get_pmic_reg(PMIC_I2C_MAXION_VOLTAGE_ADDRESS, voltage, 1));
        case MODULE_MINION:
            return (get_pmic_reg(PMIC_I2C_MINION_ALL_VOLTAGE_ADDRESS, voltage, 1));
        case MODULE_PCIE:
            return (get_pmic_reg(PMIC_I2C_PCIE_VOLTAGE_ADDRESS, voltage, 1));
        case MODULE_NOC:
            return (get_pmic_reg(PMIC_I2C_NOC_VOLTAGE_ADDRESS, voltage, 1));
        case MODULE_PCIE_LOGIC:
            return (get_pmic_reg(PMIC_I2C_PCIE_LOGIC_VOLTAGE_ADDRESS, voltage, 1));
        case MODULE_VDDQLP:
            return (get_pmic_reg(PMIC_I2C_VDDQLP_VOLTAGE_ADDRESS, voltage, 1));
        case MODULE_VDDQ:
            return (get_pmic_reg(PMIC_I2C_VDDQ_VOLTAGE_ADDRESS, voltage, 1));
        default: {
            MESSAGE_ERROR("Error invalid voltage type to extract Voltage");
            return ERROR_PMIC_I2C_INVALID_VOLTAGE_TYPE;
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_set_voltage
*
*   DESCRIPTION
*
*       This function writes specific voltage setting.
*
*   INPUTS
*
*       voltage_type      voltage type to be set
*       voltage           voltage value to be set
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_set_voltage(module_e voltage_type, uint8_t voltage)
{
    switch (voltage_type)
    {
        case MODULE_DDR:
            return (set_pmic_reg(PMIC_I2C_DDR_VOLTAGE_ADDRESS,
                                 PMIC_I2C_DDR_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case MODULE_L2CACHE:
            return (set_pmic_reg(PMIC_I2C_L2_CACHE_VOLTAGE_ADDRESS,
                                 PMIC_I2C_L2_CACHE_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case MODULE_MAXION:
            return (set_pmic_reg(PMIC_I2C_MAXION_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MAXION_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case MODULE_MINION:
            return (set_pmic_reg(PMIC_I2C_MINION_ALL_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_ALL_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case MODULE_PCIE:
            return (set_pmic_reg(PMIC_I2C_PCIE_VOLTAGE_ADDRESS,
                                 PMIC_I2C_PCIE_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case MODULE_NOC:
            return (set_pmic_reg(PMIC_I2C_NOC_VOLTAGE_ADDRESS,
                                 PMIC_I2C_NOC_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case MODULE_PCIE_LOGIC:
            return (set_pmic_reg(PMIC_I2C_PCIE_LOGIC_VOLTAGE_ADDRESS,
                                 PMIC_I2C_PCIE_LOGIC_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case MODULE_VDDQLP:
            return (set_pmic_reg(PMIC_I2C_VDDQLP_VOLTAGE_ADDRESS,
                                 PMIC_I2C_VDDQLP_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case MODULE_VDDQ:
            return (set_pmic_reg(PMIC_I2C_VDDQ_VOLTAGE_ADDRESS,
                                 PMIC_I2C_VDDQ_VOLTAGE_VOLTAGE_SET(voltage), 1));
        default: {
            MESSAGE_ERROR("Error invalid voltage type to set Voltage");
            return ERROR_PMIC_I2C_INVALID_VOLTAGE_TYPE;
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_minion_group_voltage
*
*   DESCRIPTION
*
*       This function returns minion group voltage setting.
*
*   INPUTS
*
*       group_id      minion group (1-17) voltage to be read
*
*   OUTPUTS
*
*       voltage       voltage value in mV
*
***********************************************************************/

int pmic_get_minion_group_voltage(uint8_t group_id, uint8_t *voltage)
{
    switch (group_id)
    {
        case 1:
            return (get_pmic_reg(PMIC_I2C_MINION_G1_VOLTAGE_ADDRESS, voltage, 1));
        case 2:
            return (get_pmic_reg(PMIC_I2C_MINION_G2_VOLTAGE_ADDRESS, voltage, 1));
        case 3:
            return (get_pmic_reg(PMIC_I2C_MINION_G3_VOLTAGE_ADDRESS, voltage, 1));
        case 4:
            return (get_pmic_reg(PMIC_I2C_MINION_G4_VOLTAGE_ADDRESS, voltage, 1));
        case 5:
            return (get_pmic_reg(PMIC_I2C_MINION_G5_VOLTAGE_ADDRESS, voltage, 1));
        case 6:
            return (get_pmic_reg(PMIC_I2C_MINION_G6_VOLTAGE_ADDRESS, voltage, 1));
        case 7:
            return (get_pmic_reg(PMIC_I2C_MINION_G7_VOLTAGE_ADDRESS, voltage, 1));
        case 8:
            return (get_pmic_reg(PMIC_I2C_MINION_G8_VOLTAGE_ADDRESS, voltage, 1));
        case 9:
            return (get_pmic_reg(PMIC_I2C_MINION_G9_VOLTAGE_ADDRESS, voltage, 1));
        case 10:
            return (get_pmic_reg(PMIC_I2C_MINION_G10_VOLTAGE_ADDRESS, voltage, 1));
        case 11:
            return (get_pmic_reg(PMIC_I2C_MINION_G11_VOLTAGE_ADDRESS, voltage, 1));
        case 12:
            return (get_pmic_reg(PMIC_I2C_MINION_G12_VOLTAGE_ADDRESS, voltage, 1));
        case 13:
            return (get_pmic_reg(PMIC_I2C_MINION_G13_VOLTAGE_ADDRESS, voltage, 1));
        case 14:
            return (get_pmic_reg(PMIC_I2C_MINION_G14_VOLTAGE_ADDRESS, voltage, 1));
        case 15:
            return (get_pmic_reg(PMIC_I2C_MINION_G15_VOLTAGE_ADDRESS, voltage, 1));
        case 16:
            return (get_pmic_reg(PMIC_I2C_MINION_G16_VOLTAGE_ADDRESS, voltage, 1));
        case 17:
            return (get_pmic_reg(PMIC_I2C_MINION_G17_VOLTAGE_ADDRESS, voltage, 1));
        default: {
            MESSAGE_ERROR("Error invalid minion group to extract Voltage");
            return ERROR_PMIC_I2C_INVALID_MINION_GROUP;
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_set_minion_group_voltage
*
*   DESCRIPTION
*
*       This function writes minion group voltage setting.
*
*   INPUTS
*
*       group_id          minion group voltage to be set
*       voltage           voltage value to be set
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_set_minion_group_voltage(uint8_t group_id, uint8_t voltage)
{
    switch (group_id)
    {
        case 1:
            return (set_pmic_reg(PMIC_I2C_MINION_G1_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_G1_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case 2:
            return (set_pmic_reg(PMIC_I2C_MINION_G2_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_G2_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case 3:
            return (set_pmic_reg(PMIC_I2C_MINION_G3_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_G3_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case 4:
            return (set_pmic_reg(PMIC_I2C_MINION_G4_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_G4_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case 5:
            return (set_pmic_reg(PMIC_I2C_MINION_G5_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_G5_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case 6:
            return (set_pmic_reg(PMIC_I2C_MINION_G6_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_G6_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case 7:
            return (set_pmic_reg(PMIC_I2C_MINION_G7_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_G7_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case 8:
            return (set_pmic_reg(PMIC_I2C_MINION_G8_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_G8_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case 9:
            return (set_pmic_reg(PMIC_I2C_MINION_G9_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_G9_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case 10:
            return (set_pmic_reg(PMIC_I2C_MINION_G10_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_G10_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case 11:
            return (set_pmic_reg(PMIC_I2C_MINION_G11_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_G11_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case 12:
            return (set_pmic_reg(PMIC_I2C_MINION_G12_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_G12_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case 13:
            return (set_pmic_reg(PMIC_I2C_MINION_G13_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_G13_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case 14:
            return (set_pmic_reg(PMIC_I2C_MINION_G14_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_G14_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case 15:
            return (set_pmic_reg(PMIC_I2C_MINION_G15_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_G15_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case 16:
            return (set_pmic_reg(PMIC_I2C_MINION_G16_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_G16_VOLTAGE_VOLTAGE_SET(voltage), 1));
        case 17:
            return (set_pmic_reg(PMIC_I2C_MINION_G17_VOLTAGE_ADDRESS,
                                 PMIC_I2C_MINION_G17_VOLTAGE_VOLTAGE_SET(voltage), 1));
        default: {
            MESSAGE_ERROR("Error invalid minion group to set Voltage");
            return ERROR_PMIC_I2C_INVALID_MINION_GROUP;
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_pmb_stats
*
*   DESCRIPTION
*
*       This function returns pmb stats for modules.
*
*   INPUTS
*
*       module_type      module type to be read (MINION, NOC, SRAM)
*       reading_type     output value type to be set (CURRENT, MIN, MAX, AVG)
*
*   OUTPUTS
*
*       current           current value in mA
*
***********************************************************************/
int pmic_get_pmb_stats(struct pmb_stats_t *pmb_stats)
{
    int32_t status = STATUS_SUCCESS;

    /* write PMB register to generate snapshot of all stats and then read all stats*/
    status = set_pmic_reg(PMIC_I2C_PMB_RW_ADDRESS, PMIC_I2C_PMB_STATS_SNAPSHOT_VALUE, 1);

    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->minion.v_out);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->minion.a_out);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->minion.w_out);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->minion.v_in);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->minion.a_in);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->minion.w_in);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->minion.deg_c);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->noc.v_out);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->noc.a_out);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->noc.w_out);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->noc.v_in);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->noc.a_in);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->noc.w_in);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->noc.deg_c);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->sram.v_out);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->sram.a_out);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->sram.w_out);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->sram.v_in);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->sram.a_in);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->sram.w_in);
    }
    if (status == STATUS_SUCCESS)
    {
        status = pmic_get_pmb_rstat(&pmb_stats->sram.deg_c);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_enable_wdog_timer
*
*   DESCRIPTION
*
*       This function enables the watchdog timer.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_enable_wdog_timer(void)
{
    uint8_t reg_value;

    if (0 != get_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    return (set_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS,
                         PMIC_I2C_WDOG_TIMER_CONFIG_WDT_ENABLE_MODIFY(reg_value, 1), 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_disable_wdog_timer
*
*   DESCRIPTION
*
*       This function disables the watchdog timer.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_disable_wdog_timer(void)
{
    uint8_t reg_value;

    if (0 != get_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    return (set_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS,
                         PMIC_I2C_WDOG_TIMER_CONFIG_WDT_ENABLE_MODIFY(reg_value, 0), 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_enable_wdog_timeout_reset
*
*   DESCRIPTION
*
*       This function enables timeout to assert RESET, 0 to assert PERST only.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_enable_wdog_timeout_reset(void)
{
    uint8_t reg_value;

    if (0 != get_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    return (set_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS,
                         PMIC_I2C_WDOG_TIMER_CONFIG_WTD_ASSERT_MODIFY(reg_value, 1), 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_disable_wdog_timeout_reset
*
*   DESCRIPTION
*
*       This function disables timeout to assert RESET, 0 to assert PERST only.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_disable_wdog_timeout_reset(void)
{
    uint8_t reg_value;

    if (0 != get_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    return (set_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS,
                         PMIC_I2C_WDOG_TIMER_CONFIG_WTD_ASSERT_MODIFY(reg_value, 0), 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_wdog_timeout_time
*
*   DESCRIPTION
*
*       This function gets watchdog timeout time.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       wdog_time      wdog timeout time in miliseconds
*
***********************************************************************/

int pmic_get_wdog_timeout_time(uint32_t *wdog_time)
{
    uint8_t reg_value;
    uint8_t timeout_time;

    if (0 != get_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    timeout_time = PMIC_I2C_WDOG_TIMER_CONFIG_WDT_TIME_GET(reg_value);

    *wdog_time = (uint32_t)(timeout_time * 200);

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_set_wdog_timeout_time
*
*   DESCRIPTION
*
*       This function sets watchdog timeout time.
*
*   INPUTS
*
*       timeout_time    timeout time in miliseconds (multiple of 200ms)
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_set_wdog_timeout_time(uint32_t timeout_time)
{
    uint8_t reg_value;
    uint8_t new_reg_value;
    uint8_t new_timeout_value;

    if (0 != get_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }
    new_timeout_value = (uint8_t)(timeout_time / 200);
    new_reg_value = (uint8_t)((reg_value & 0x0F0u) | (new_timeout_value & 0x0Fu));

    return (set_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS, new_reg_value, 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_tdp_threshold
*
*   DESCRIPTION
*
*       This function gets power alarm point.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       power_alarm      power alarm point
*
***********************************************************************/

int pmic_get_tdp_threshold(uint8_t *power_alarm)
{
    return (get_pmic_reg(PMIC_I2C_PWR_ALARM_CONFIG_ADDRESS, power_alarm, 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_set_tdp_threshold
*
*   DESCRIPTION
*
*       This function sets power alarm point.
*
*   INPUTS
*
*       power_alarm      power alarm value
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_set_tdp_threshold(uint8_t power_alarm)
{
    return (set_pmic_reg(PMIC_I2C_PWR_ALARM_CONFIG_ADDRESS, power_alarm, 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_force_shutdown
*
*   DESCRIPTION
*
*       This function forces a shutdown, only a power cycle will allow the system to turn on.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_force_shutdown(void)
{
    return (set_pmic_reg(PMIC_I2C_RESET_COMMAND_ADDRESS, PMIC_I2C_RESET_COMMAND_SHUTDOWN, 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_force_power_off_on
*
*   DESCRIPTION
*
*       This function force a power off/on cycle which shuts down and restarts the system.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_force_power_off_on(void)
{
    return (set_pmic_reg(PMIC_I2C_RESET_COMMAND_ADDRESS, PMIC_I2C_RESET_COMMAND_PWR_CYCLE, 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_force_reset
*
*   DESCRIPTION
*
*       This function forces a RESET to the ET-SOC
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_force_reset(void)
{
    return (set_pmic_reg(PMIC_I2C_RESET_COMMAND_ADDRESS, PMIC_I2C_RESET_COMMAND_RESET, 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_force_perst
*
*   DESCRIPTION
*
*       This function forces a PERST reset to the ET-SOC
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_force_perst(void)
{
    return (set_pmic_reg(PMIC_I2C_RESET_COMMAND_ADDRESS, PMIC_I2C_RESET_COMMAND_PERST, 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_reset_wdog_timer
*
*   DESCRIPTION
*
*       This function resets watchdog timer
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int pmic_reset_wdog_timer(void)
{
    uint8_t reg_value;

    if (0 != get_pmic_reg(PMIC_I2C_WDOG_TIMER_RESET_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    return (set_pmic_reg(PMIC_I2C_WDOG_TIMER_RESET_ADDRESS,
                         PMIC_I2C_WDOG_TIMER_RESET_WDT_POKE_MODIFY(reg_value, 1), 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_read_average_soc_power
*
*   DESCRIPTION
*
*       This function gets Average Power in mW.
*
*   INPUTS
*
*       uint8_t  Pointer to data to load result to
*
*   OUTPUTS
*
*       avg_power     value of Average Power in binary encoded value.
*
***********************************************************************/

int pmic_read_average_soc_power(uint16_t *avg_power)
{
    int32_t status;

    /* Read avg soc power form PMIC */
    status = get_pmic_reg(PMIC_I2C_AVERAGE_PWR_ADDRESS, (uint8_t *)avg_power, 2);
    if (status == STATUS_SUCCESS)
    {
        /* Power is in 10 mW units as defined in PMIC docs*/
        *avg_power = (uint16_t)((*avg_power) * 10);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       I2C_PMIC_Read
*
*   DESCRIPTION
*
*       This function reads value in PMIC register.
*
*   INPUTS
*
*       reg     register to read from
*
*   OUTPUTS
*
*       value stored in register
*
***********************************************************************/
int I2C_PMIC_Read(uint8_t reg)
{
    uint8_t reg_value;

    if (0 != get_pmic_reg(reg, &reg_value, 1))
    {
        MESSAGE_ERROR("PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    return reg_value;
}

/************************************************************************
*
*   FUNCTION
*
*       I2C_PMIC_Write
*
*   DESCRIPTION
*
*       This function writes value in PMIC register.
*
*   INPUTS
*
*       reg     register to write into
*
*   OUTPUTS
*
*       status of function call
*
***********************************************************************/
int I2C_PMIC_Write(uint8_t reg, uint8_t data)
{
    return set_pmic_reg(reg, data, 1);
}
