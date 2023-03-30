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
        pmic_reset_pmb_stats
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
#include "bl2_scratch_buffer.h"
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

/* PMIC firmware update defines (timeouts wait up to 3x the expected time) */
#define FW_UPDATE_WAIT_FACTOR      3
#define FW_UPDATE_START_TIMEOUT_MS (6 * FW_UPDATE_WAIT_FACTOR)
#define FW_UPDATE_PAGE_TIMEOUT_MS  (3 * FW_UPDATE_WAIT_FACTOR)
#define FW_UPDATE_ROW_TIMEOUT_MS   (9 * FW_UPDATE_WAIT_FACTOR)
#define FW_UPDATE_CKSUM_TIMEOUT_MS (21 * FW_UPDATE_WAIT_FACTOR)
#define FW_UPDATE_FLASH_ROW_SIZE   256
#define FW_UPDATE_FLASH_PAGE_SIZE  64

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
*       Wait PMIC Ready
*
*   DESCRIPTION
*
*       This function does a timed wait for PMIC ready to assert.  Profiling
*       shows PMIC ready is de-asserted a maximum of ~0.5 milliseconds with
*       ~0.1 milliseconds the common case.  The timeout value used here has
*       ample slack.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       status   success/error
*
***********************************************************************/
int wait_pmic_ready(void)
{
#if !FAST_BOOT
    uint64_t elapsed_ms;
    const uint64_t start_ticks = timer_get_ticks_count();
    do
    {
        /* GPIO 15 is designated as PMIC Ready indication */
        if (gpio_read_pin_value(GPIO_CONTROLLER_ID_SPIO, 15))
        {
            return 0;
        }

        elapsed_ms = timer_convert_ticks_to_ms(timer_get_ticks_count() - start_ticks);
    } while (elapsed_ms < PMIC_BUSY_WAIT_TIMEOUT_MS);

    Log_Write(LOG_LEVEL_WARNING, "wait_pmic_ready timed out after %u ms\n",
              PMIC_BUSY_WAIT_TIMEOUT_MS);
    return ERROR_PMIC_WAIT_READY_TIMEOUT;
#else
    return 0;
#endif
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
        MESSAGE_ERROR("get_pmic_reg: PMIC read reg: %d failed!", reg);
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
*       value      pointer to value to be written
*       reg_size   register size in bytes
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

inline static int set_pmic_reg(uint8_t reg, const uint8_t *value, uint8_t reg_size)
{
    if (0 != i2c_write(&g_pmic_i2c_dev_reg, reg, value, reg_size))
    {
        MESSAGE_ERROR("set_pmic_reg: PMIC write failed!");
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
    return set_pmic_reg(PMIC_I2C_INT_CTRL_ADDRESS, &int_cfg, 1);
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
        MESSAGE_ERROR("pmic_error_isr: PMIC read int cause failed!");
    }

    /* Generate PMIC Error */

    if (PMIC_I2C_INT_CAUSE_OV_TEMP_GET(int_cause))
    {
        event_control_block.thermal_pwr_event_cb(int_cause);

        if (0 != pmic_get_temperature(&reg_value))
        {
            MESSAGE_ERROR("pmic_error_isr: PMIC get_temperature failed!");
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
            MESSAGE_ERROR("pmic_error_isr: PMIC read instantenous soc power failed!");
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
            MESSAGE_ERROR("pmic_error_isr: PMIC get_input_voltage failed!");
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
            MESSAGE_ERROR("pmic_error_isr: PMIC get_command_comm_fault failed!");
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
            MESSAGE_ERROR("pmic_error_isr: PMIC get_reg_comm_fault failed!");
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
            MESSAGE_ERROR("pmic_error_isr: PMIC get_reg_fault failed!");
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
*       pmic_get_fw_src_hash
*
*   DESCRIPTION
*
*       This function reads GIT HASH of the current PMIC version.
*
*   INPUTS
*
*       fw_src_hash Pointer to return value of source HASH
*
*   OUTPUTS
*
*       fw_src_hash  GIT HASH value of the PMIC Firmware source.
*
***********************************************************************/

int pmic_get_fw_src_hash(uint32_t *fw_src_hash)
{
    return (get_pmic_reg(PMIC_I2C_FIRMWARE_SRC_HASH_ADDRESS, (uint8_t *)fw_src_hash, 4));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_fw_version
*
*   DESCRIPTION
*
*       This function returns the PMIC FW SEM version.
*
*   INPUTS
*
*       Pointer to major, minor and patch
*
*   OUTPUTS
*
*       fw_sem_ver  SEM Version of the current PMIC Firmware.
*
***********************************************************************/
int pmic_get_fw_version(uint8_t *major, uint8_t *minor, uint8_t *patch)
{
    uint32_t fw_sem_ver;
    uint8_t val = PMIC_I2C_FW_MGMTCMD_VERSION;

    /* write SUB_CMD:05 */
    set_pmic_reg(PMIC_I2C_FW_MGMTCMD_ADDRESS, &val, 1);

    /* read data back */
    if (0 != get_pmic_reg(PMIC_I2C_FW_MGMTDATA_ADDRESS, (uint8_t *)&fw_sem_ver, 4))
    {
        MESSAGE_ERROR("pmic_get_fw_version: PMIC read reg failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    *major = (fw_sem_ver & 0xFF0000) >> 16;
    *minor = (fw_sem_ver & 0xFF00) >> 8;
    *patch = (fw_sem_ver & 0xFF);

    return SUCCESS;
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
*       Board_type Pointer to value
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
    uint8_t val;

    if (index > 7)
    {
        MESSAGE_ERROR("Index out of range!");
        return ERROR_PMIC_I2C_INVALID_ARGUMENTS;
    }

    if (0 != get_pmic_reg(PMIC_I2C_GPIO_CONFIG_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("pmic_set_gpio_as_input: PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    reg_value = reg_value & (uint8_t)(~(0x1u << index));
    val = PMIC_I2C_GPIO_CONFIG_INPUT_OUTPUT_SET(reg_value);

    return (set_pmic_reg(PMIC_I2C_GPIO_CONFIG_ADDRESS, &val, 1));
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
    uint8_t val;

    if (index > 7)
    {
        MESSAGE_ERROR("Index out of range!");
        return ERROR_PMIC_I2C_INVALID_ARGUMENTS;
    }

    if (0 != get_pmic_reg(PMIC_I2C_GPIO_CONFIG_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("pmic_set_gpio_as_output: PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    reg_value = reg_value | (uint8_t)(0x1u << index);
    val = PMIC_I2C_GPIO_CONFIG_INPUT_OUTPUT_SET(reg_value);

    return (set_pmic_reg(PMIC_I2C_GPIO_CONFIG_ADDRESS, &val, 1));
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
        MESSAGE_ERROR("pmic_get_gpio_bit: PMIC read failed");
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
    uint8_t val;

    if (index > 7)
    {
        MESSAGE_ERROR("Index out of range!");
        return ERROR_PMIC_I2C_INVALID_ARGUMENTS;
    }

    if (0 != get_pmic_reg(PMIC_I2C_GPIO_RW_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("pmic_set_gpio_bit: PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    reg_value = reg_value | (uint8_t)(0x1u << index);
    val = PMIC_I2C_GPIO_RW_INPUT_OUTPUT_SET(reg_value);

    return (set_pmic_reg(PMIC_I2C_GPIO_RW_ADDRESS, &val, 1));
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
    uint8_t val;

    if (index > 7)
    {
        MESSAGE_ERROR("Index out of range!");
        return ERROR_PMIC_I2C_INVALID_ARGUMENTS;
    }

    if (0 != get_pmic_reg(PMIC_I2C_GPIO_RW_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("pmic_clear_gpio_bit: PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    reg_value = reg_value & (uint8_t)(~(0x1u << index));
    val = PMIC_I2C_GPIO_RW_INPUT_OUTPUT_SET(reg_value);

    return (set_pmic_reg(PMIC_I2C_GPIO_RW_ADDRESS, &val, 1));
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
        return set_pmic_reg(PMIC_I2C_TEMP_ALARM_CONF_ADDRESS, &temp_limit, 1);
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
*       This function returns soc input power in 10 mW steps.
*
*   INPUTS
*
*       soc_pwr_10mW  Pointer to data to load result to
*
*   OUTPUTS
*
*       status        The function call status, pass/fail.
*
***********************************************************************/

int pmic_read_instantaneous_soc_power(uint16_t *soc_pwr_10mW)
{
    /* Read input power from PMIC */
    return get_pmic_reg(PMIC_I2C_INPUT_POWER_ADDRESS, (uint8_t *)soc_pwr_10mW, 2);
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
    uint8_t val;

    if (0 != get_pmic_reg(PMIC_I2C_RESET_CTRL_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("pmic_enable_etsoc_reset_after_perst: PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    val = PMIC_I2C_RESET_CTRL_PERST_EN_MODIFY(reg_value, 1);
    return (set_pmic_reg(PMIC_I2C_RESET_CTRL_ADDRESS, &val, 1));
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
    uint8_t val;

    if (0 != get_pmic_reg(PMIC_I2C_RESET_CTRL_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("pmic_disable_etsoc_reset_after_perst: PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    val = PMIC_I2C_RESET_CTRL_PERST_EN_MODIFY(reg_value, 0);
    return (set_pmic_reg(PMIC_I2C_RESET_CTRL_ADDRESS, &val, 1));
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
    uint8_t val;

    switch (voltage_type)
    {
        case MODULE_DDR:
            val = PMIC_I2C_DDR_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_DDR_VOLTAGE_ADDRESS, &val, 1));
        case MODULE_L2CACHE:
            val = PMIC_I2C_L2_CACHE_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_L2_CACHE_VOLTAGE_ADDRESS, &val, 1));
        case MODULE_MAXION:
            val = PMIC_I2C_MAXION_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MAXION_VOLTAGE_ADDRESS, &val, 1));
        case MODULE_MINION:
            val = PMIC_I2C_MINION_ALL_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_ALL_VOLTAGE_ADDRESS, &val, 1));
        case MODULE_PCIE:
            val = PMIC_I2C_PCIE_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_PCIE_VOLTAGE_ADDRESS, &val, 1));
        case MODULE_NOC:
            val = PMIC_I2C_NOC_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_NOC_VOLTAGE_ADDRESS, &val, 1));
        case MODULE_PCIE_LOGIC:
            val = PMIC_I2C_PCIE_LOGIC_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_PCIE_LOGIC_VOLTAGE_ADDRESS, &val, 1));
        case MODULE_VDDQLP:
            val = PMIC_I2C_VDDQLP_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_VDDQLP_VOLTAGE_ADDRESS, &val, 1));
        case MODULE_VDDQ:
            val = PMIC_I2C_VDDQ_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_VDDQ_VOLTAGE_ADDRESS, &val, 1));
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
    uint8_t val;

    switch (group_id)
    {
        case 1:
            val = PMIC_I2C_MINION_G1_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_G1_VOLTAGE_ADDRESS, &val, 1));
        case 2:
            val = PMIC_I2C_MINION_G2_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_G2_VOLTAGE_ADDRESS, &val, 1));
        case 3:
            val = PMIC_I2C_MINION_G3_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_G3_VOLTAGE_ADDRESS, &val, 1));
        case 4:
            val = PMIC_I2C_MINION_G4_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_G4_VOLTAGE_ADDRESS, &val, 1));
        case 5:
            val = PMIC_I2C_MINION_G5_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_G5_VOLTAGE_ADDRESS, &val, 1));
        case 6:
            val = PMIC_I2C_MINION_G6_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_G6_VOLTAGE_ADDRESS, &val, 1));
        case 7:
            val = PMIC_I2C_MINION_G7_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_G7_VOLTAGE_ADDRESS, &val, 1));
        case 8:
            val = PMIC_I2C_MINION_G8_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_G8_VOLTAGE_ADDRESS, &val, 1));
        case 9:
            val = PMIC_I2C_MINION_G9_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_G9_VOLTAGE_ADDRESS, &val, 1));
        case 10:
            val = PMIC_I2C_MINION_G10_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_G10_VOLTAGE_ADDRESS, &val, 1));
        case 11:
            val = PMIC_I2C_MINION_G11_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_G11_VOLTAGE_ADDRESS, &val, 1));
        case 12:
            val = PMIC_I2C_MINION_G12_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_G12_VOLTAGE_ADDRESS, &val, 1));
        case 13:
            val = PMIC_I2C_MINION_G13_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_G13_VOLTAGE_ADDRESS, &val, 1));
        case 14:
            val = PMIC_I2C_MINION_G14_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_G14_VOLTAGE_ADDRESS, &val, 1));
        case 15:
            val = PMIC_I2C_MINION_G15_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_G15_VOLTAGE_ADDRESS, &val, 1));
        case 16:
            val = PMIC_I2C_MINION_G16_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_G16_VOLTAGE_ADDRESS, &val, 1));
        case 17:
            val = PMIC_I2C_MINION_G17_VOLTAGE_VOLTAGE_SET(voltage);
            return (set_pmic_reg(PMIC_I2C_MINION_G17_VOLTAGE_ADDRESS, &val, 1));
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
*       pmic_reset_pmb_stats
*
*   DESCRIPTION
*
*       This function resets pmb stats for modules.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       status           status of reset success/error
*
***********************************************************************/
int pmic_reset_pmb_stats(void)
{
    uint8_t val = PMIC_I2C_PMB_STATS_RESET_VALUE;
    /* write PMB register to reset all stats */
    return set_pmic_reg(PMIC_I2C_PMB_RW_ADDRESS, &val, 1);
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
*       pmb_stats        stats to update
*
*   OUTPUTS
*
*       status           status of update success/error
*
***********************************************************************/
int pmic_get_pmb_stats(struct pmb_stats_t *pmb_stats)
{
    int32_t status = STATUS_SUCCESS;
    uint32_t val = PMIC_I2C_PMB_STATS_SNAPSHOT_VALUE;
    /* write PMB register to generate snapshot of all stats and then read all stats*/
    status = set_pmic_reg(PMIC_I2C_PMB_RW_ADDRESS, (uint8_t *)&val, 4);

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
    uint8_t val;

    if (0 != get_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("pmic_enable_wdog_timer: PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    val = PMIC_I2C_WDOG_TIMER_CONFIG_WDT_ENABLE_MODIFY(reg_value, 1);
    return (set_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS, &val, 1));
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
    uint8_t val;

    if (0 != get_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("pmic_disable_wdog_timer: PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    val = PMIC_I2C_WDOG_TIMER_CONFIG_WDT_ENABLE_MODIFY(reg_value, 0);
    return (set_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS, &val, 1));
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
    uint8_t val;

    if (0 != get_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("pmic_enable_wdog_timeout_reset: PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    val = PMIC_I2C_WDOG_TIMER_CONFIG_WTD_ASSERT_MODIFY(reg_value, 1);
    return (set_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS, &val, 1));
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
    uint8_t val;

    if (0 != get_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("pmic_disable_wdog_timeout_reset: PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    val = PMIC_I2C_WDOG_TIMER_CONFIG_WTD_ASSERT_MODIFY(reg_value, 0);
    return (set_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS, &val, 1));
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
        MESSAGE_ERROR("pmic_get_wdog_timeout_time: PMIC read failed");
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
        MESSAGE_ERROR("pmic_set_wdog_timeout_time: PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }
    new_timeout_value = (uint8_t)(timeout_time / 200);
    new_reg_value = (uint8_t)((reg_value & 0x0F0u) | (new_timeout_value & 0x0Fu));

    return (set_pmic_reg(PMIC_I2C_WDOG_TIMER_CONFIG_ADDRESS, &new_reg_value, 1));
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
    return (set_pmic_reg(PMIC_I2C_PWR_ALARM_CONFIG_ADDRESS, &power_alarm, 1));
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
    uint8_t val = PMIC_I2C_RESET_COMMAND_SHUTDOWN;
    return (set_pmic_reg(PMIC_I2C_RESET_COMMAND_ADDRESS, &val, 1));
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
    uint8_t val = PMIC_I2C_RESET_COMMAND_PWR_CYCLE;
    return (set_pmic_reg(PMIC_I2C_RESET_COMMAND_ADDRESS, &val, 1));
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
    uint8_t val = PMIC_I2C_RESET_COMMAND_RESET;
    return (set_pmic_reg(PMIC_I2C_RESET_COMMAND_ADDRESS, &val, 1));
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
    uint8_t val = PMIC_I2C_RESET_COMMAND_PERST;
    return (set_pmic_reg(PMIC_I2C_RESET_COMMAND_ADDRESS, &val, 1));
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
    uint8_t val;

    if (0 != get_pmic_reg(PMIC_I2C_WDOG_TIMER_RESET_ADDRESS, &reg_value, 1))
    {
        MESSAGE_ERROR("pmic_reset_wdog_timer: PMIC read failed");
        return ERROR_PMIC_I2C_READ_FAILED;
    }

    val = PMIC_I2C_WDOG_TIMER_RESET_WDT_POKE_MODIFY(reg_value, 1);
    return (set_pmic_reg(PMIC_I2C_WDOG_TIMER_RESET_ADDRESS, &val, 1));
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_read_average_soc_power
*
*   DESCRIPTION
*
*       This function gets Average Power in 10 mW steps.
*
*   INPUTS
*
*       avg_pwr_10mw  Pointer to data to load result to
*
*   OUTPUTS
*
*       status        Success or error code.
*
***********************************************************************/

int pmic_read_average_soc_power(uint16_t *avg_pwr_10mw)
{
    /* Read avg soc power form PMIC */
    return get_pmic_reg(PMIC_I2C_AVERAGE_PWR_ADDRESS, (uint8_t *)avg_pwr_10mw, 2);
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_get_inactive_boot_slot
*
*   DESCRIPTION
*
*       This function gets the boot slot not currently running.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       slot      boot slot not currently running
*
***********************************************************************/

static int pmic_get_inactive_boot_slot(uint8_t *slot)
{
    int status;
    uint32_t val;
    uint8_t cmd = PMIC_I2C_FW_MGMTCMD_ACTIVE_SLOT;

    if (!slot)
    {
        MESSAGE_ERROR("Error slot argument is null\n");
        return ERROR_PMIC_I2C_INVALID_ARGUMENTS;
    }

    status = set_pmic_reg(PMIC_I2C_FW_MGMTCMD_ADDRESS, &cmd, 1);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = get_pmic_reg(PMIC_I2C_FW_MGMTDATA_ADDRESS, (uint8_t *)&val, 4);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    *slot = (uint8_t)!val;
    return STATUS_SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_wait_for_flash_ready
*
*   DESCRIPTION
*
*       This function waits up to the timeout for the pmic flash to be
*       ready (busy bit is clear) while checking for an error.  PMIC_READY
*       is also checked prior to entering the timed loop since it is
*       deasserted until the busy bit is set.  This ensures the busy bit
*       status is accurate.
*
*   INPUTS
*
*       timeout_ms timeout value in milliseconds
*
*   OUTPUTS
*
*       status     Success or error code.
*
***********************************************************************/

static int pmic_wait_for_flash_ready(uint64_t timeout_ms)
{
    bool busy;
    uint8_t value;
    uint64_t elapsed_ms;
    uint64_t start_ticks;

    if (wait_pmic_ready() != 0)
    {
        return ERROR_PMIC_I2C_FW_MGMTCMD_TIMEOUT;
    }

    start_ticks = timer_get_ticks_count();

    do
    {
        int status = get_pmic_reg(PMIC_I2C_FW_MGMTCMD_ADDRESS, &value, 1);

        if (status != STATUS_SUCCESS)
        {
            return status;
        }

        if ((value & PMIC_I2C_FW_MGMTCMD_STATUS_ERROR) != 0)
        {
            Log_Write(LOG_LEVEL_CRITICAL, "pmic status error\n");
            return ERROR_PMIC_I2C_FW_MGMTCMD_ILLEGAL;
        }

        busy = (value & PMIC_I2C_FW_MGMTCMD_STATUS_BUSY) != 0;
        elapsed_ms = timer_convert_ticks_to_ms(timer_get_ticks_count() - start_ticks);
    } while (busy && elapsed_ms < timeout_ms);

    if (busy)
    {
        Log_Write(LOG_LEVEL_ERROR, "pmic_wait_for_flash_ready timed out %ld ms\n", timeout_ms);
        return ERROR_PMIC_I2C_FW_MGMTCMD_TIMEOUT;
    }

    return STATUS_SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_send_firmware_block
*
*   DESCRIPTION
*
*       This function sends a block of firmware to the pmic 4 bytes at a time.
*
*   INPUTS
*
*       flash_addr    current flash addr
*       fw_ptr        current fw image ptr
*       fw_block_size size in bytes of the next block of fw to send
*
*   OUTPUTS
*
*       status        Success or error code.
*
***********************************************************************/

static int pmic_send_firmware_block(uint32_t flash_addr, uint8_t *fw_ptr, uint32_t fw_block_size)
{
    int status = STATUS_SUCCESS;
    uint32_t write_count;
    uint64_t flash_wait_ms = flash_addr == 0 ? FW_UPDATE_START_TIMEOUT_MS : 0;

    for (write_count = 0; write_count < fw_block_size / 4; write_count++)
    {
        if (flash_wait_ms > 0)
        {
            status = pmic_wait_for_flash_ready(flash_wait_ms);
        }

        if (status != STATUS_SUCCESS)
        {
            break;
        }

        status = set_pmic_reg(PMIC_I2C_FW_MGMTDATA_ADDRESS, fw_ptr, 4);
        if (status != STATUS_SUCCESS)
        {
            break;
        }

        if ((flash_addr & (FW_UPDATE_FLASH_ROW_SIZE - 1)) == 0)
        {
            flash_wait_ms = FW_UPDATE_ROW_TIMEOUT_MS;
        }
        else if ((flash_addr & (FW_UPDATE_FLASH_PAGE_SIZE - 1)) == 0)
        {
            flash_wait_ms = FW_UPDATE_PAGE_TIMEOUT_MS;
        }
        else
        {
            flash_wait_ms = 0;
        }

        fw_ptr += 4;
        flash_addr += 4;
    }

    if (status != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] PMIC programming failed (write_count %u)\n",
                  flash_addr / 4);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       pmic_firmware_update
*
*   DESCRIPTION
*
*       This function updates the pmic firmware
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       match   pmic update version and currently running version match
*
***********************************************************************/
int pmic_firmware_update(bool *match)
{
    int status;
    uint8_t cmd;
    uint8_t slot;
    uint32_t i;
    uint32_t sum;
    uint32_t cksum_result;
    uint32_t flash_addr = 0;
    uint32_t scratch_buffer_size = 0;
    uint64_t start;
    uint64_t end;
    uint64_t prog_start;
    uint64_t prog_end;
    uint64_t verify_start;
    uint64_t verify_end;
    uint32_t *fw;
    const uint32_t fw_data_size = 1024; // 1K block of firmware to send
    const uint32_t fw_send_count = 100; // send 1K firmware block this many times (must be <= 122)
    const uint32_t fw_size = fw_data_size * fw_send_count;
    const uint32_t fw_image_location = 0x2000;

    /* Get the pointer to BL2 scratch region */
    fw = get_scratch_buffer(&scratch_buffer_size);
    if (scratch_buffer_size < fw_data_size)
    {
        return ERROR_INSUFFICIENT_MEMORY;
    }

    start = timer_get_ticks_count();

    if (!match)
    {
        MESSAGE_ERROR("Error match pointer is null\n");
        return ERROR_PMIC_I2C_INVALID_ARGUMENTS;
    }

    status = pmic_get_inactive_boot_slot(&slot);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] Programming PMIC slot %u (%u bytes)...\n", slot, fw_size);

    prog_start = timer_get_ticks_count();
    cmd = (uint8_t)(PMIC_I2C_FW_MGMTCMD_UPDATE | (slot << PMIC_I2C_FW_MGMTCMD_SLOT_LSB));
    status = set_pmic_reg(PMIC_I2C_FW_MGMTCMD_ADDRESS, &cmd, 1);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    // Create a test pattern to send
    sum = 0;
    for (i = 0; i < fw_data_size / sizeof(fw[0]); i++)
    {
        fw[i] = 0x1u << (i % 32);
        sum += fw[i];
    }
    sum *= fw_send_count;

    // The first sent firmare block has special info, so adjust it accordingly
    // TODO: Old FW convention, fix it for new layout
    sum += (fw_image_location - fw[0]) + (fw_size - fw[2]) - fw[3];
    fw[0] = fw_image_location; // loadable image location
    fw[2] = fw_size;           // loadable image length
    fw[3] = 0 - sum;           // cksum adjustment so all words in the fw image sum to zero

    flash_addr = 0;
    status = pmic_send_firmware_block(flash_addr, (uint8_t *)fw, fw_data_size);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    // Restore the original test pattern for the remaining sends
    fw[0] = 1 << 0;
    fw[2] = 1 << 2;
    fw[3] = 1 << 3;
    for (i = 1; i < fw_send_count; i++)
    {
        flash_addr += fw_data_size;
        status = pmic_send_firmware_block(flash_addr, (uint8_t *)fw, fw_data_size);
        if (status != STATUS_SUCCESS)
        {
            return status;
        }
    }

    prog_end = timer_get_ticks_count();
    Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] PMIC programmed successfully\n");

    verify_start = timer_get_ticks_count();
    cmd = (uint8_t)(PMIC_I2C_FW_MGMTCMD_CKSUMREAD | (slot << PMIC_I2C_FW_MGMTCMD_SLOT_LSB));
    status = set_pmic_reg(PMIC_I2C_FW_MGMTCMD_ADDRESS, &cmd, 1);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = pmic_wait_for_flash_ready(FW_UPDATE_CKSUM_TIMEOUT_MS);
    if (status != STATUS_SUCCESS)
    {
        MESSAGE_ERROR("pmic flash cksum wait error\n");
        return status;
    }

    /* TODO: SW-16740: Check why checksum fails */
    status = get_pmic_reg(PMIC_I2C_FW_MGMTDATA_ADDRESS, (uint8_t *)&cksum_result, 4);
    if (status == STATUS_SUCCESS && cksum_result != 1)
    {
        MESSAGE_ERROR("pmic flash cksum failure %u\n", cksum_result);
        return ERROR_PMIC_I2C_FW_MGMTCMD_CKSUM;
    }

    verify_end = timer_get_ticks_count();
    end = timer_get_ticks_count();

    Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] PMIC loaded bytes verified OK!\n");
    Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] PMIC programmed and verified successfully\n");
    Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] PMIC completed after %ld seconds\n",
              timer_convert_ticks_to_secs(end - start));
    Log_Write(LOG_LEVEL_CRITICAL, "[ETFP] PMIC OK (Total %lds, Prog %lds, Verify %lds)\n",
              timer_convert_ticks_to_secs(end - start),
              timer_convert_ticks_to_secs(prog_end - prog_start),
              timer_convert_ticks_to_secs(verify_end - verify_start));

    return STATUS_SUCCESS;
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
        MESSAGE_ERROR("I2C_PMIC_Read: PMIC read reg: %d failed", reg);
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
    return set_pmic_reg(reg, &data, 1);
}
