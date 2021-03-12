/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

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

#include "hal_device.h"
#include "rm_esr.h"
#include "io.h"

#include "bl2_i2c_driver.h"

#include "bl2_pmic_controller.h"

#define PMIC_SLAVE_ADDRESS 0x2

// ***********************************
// Generic PMIC setup
// ***********************************
static ET_I2C_DEV_t g_pmic_i2c_dev_reg;

static void set_pmic_i2c_dev(void)
{
    g_pmic_i2c_dev_reg.regs = (I2c *)R_SP_I2C0_BASEADDR;
    g_pmic_i2c_dev_reg.isInitialized = false;
}

void setup_pmic(void)
{
    set_pmic_i2c_dev();

    if (0 != i2c_init(&g_pmic_i2c_dev_reg, ET_I2C_SPEED_400k, PMIC_SLAVE_ADDRESS)) {
        printf("PMIC connection failed to establish link\n");
    }

    printf("PMIC connection establish\n");
}

int32_t pmic_error_control_init(dm_event_isr_callback event_cb)
{
    (void)event_cb;
    return 0;
}

void pmic_error_isr(void)
{
}

// ***********************************
// Generic PMIC Read/Write functions
// ***********************************
static uint8_t get_pmic_reg(uint8_t reg)
{
    uint8_t buf;
    i2c_read(&g_pmic_i2c_dev_reg, reg, &buf, 1);
    return buf;
}

static void set_pmic_reg(uint8_t reg, uint8_t value)
{
    i2c_write(&g_pmic_i2c_dev_reg, reg, &value, 1);
}

// ***********************************
// Specific Register Access
// ***********************************

uint8_t pmic_read_soc_power(void)
{
    return (get_pmic_reg(INPUT_POWER));
}

int pmic_set_temperature_threshold(_Bool reg, int limit)
{
    if ((limit < 55) || (limit > 85)) {
        printf("Error unsupported Temperature limits\n");
        return -1;
    } else {
        set_pmic_reg((reg ? TEMP_ALARM_CONFIG_LO : TEMP_ALARM_CONFIG_HI), (uint8_t)(limit - 55));
        return 0;
    }
}

void pmic_set_tdp_threshold(int limit)
{
    if ((limit < 0) || (limit > 64)) {
        printf("Error unsupported TDP limits\n");
    } else {
        set_pmic_reg(POWER_ALARM_SET_POINT, (uint8_t)(limit << 2));
    }
}

uint8_t pmic_get_temperature(void)
{
    return (get_pmic_reg(SYSTEM_TEMP));
}

uint8_t pmic_get_voltage(enum shire_type_t shire)
{
    switch (shire) {
    case DDR:
        return (get_pmic_reg(DDR_VOLTAGE));
    case L2CACHE:
        return (get_pmic_reg(L2_CACHE_VOLTAGE));
    case MAXION:
        return (get_pmic_reg(MAXION_VOLTAGE));
    case MINION:
        return (get_pmic_reg(MINION_SHIRE_ALL_VOLTAGE));
    case PCIE:
        return (get_pmic_reg(PCIE_VOLTAGE));
    case NOC:
        return (get_pmic_reg(NOC_VOLTAGE));
    default: {
        printf("Error invalid Shire ID to extract Voltage");
        return 0;
    }
    }
}
