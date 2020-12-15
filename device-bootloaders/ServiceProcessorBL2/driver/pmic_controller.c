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

// TODO: To be migrated to PMIC interface register... 
#define PMIC_SLAVE_ADDRESS 0x2
#define SOC_TEMPERATURE 0x3
#define SYSTEM_TDP 0x4
#define SOC_POWER 0x5

// ***********************************
// Generic PMIC setup
// ***********************************
static ET_I2C_DEV_t g_pmic_i2c_dev_reg;

static void set_pmic_i2c_dev(void) 
{
    g_pmic_i2c_dev_reg.regs = (I2c*)R_SP_I2C0_BASEADDR;
    g_pmic_i2c_dev_reg.isInitialized = false;
}

void setup_pmic(void)
{
    set_pmic_i2c_dev();

    if (0 != i2c_init(&g_pmic_i2c_dev_reg, ET_I2C_SPEED_400k, PMIC_SLAVE_ADDRESS)) 
    {
     printf("PMIC connection failed to establish link\n");
    }

    printf("PMIC connection establish\n");
}

// ***********************************
// Generic PMIC Read/Write functions
// ***********************************
static uint8_t get_pmic_reg(uint8_t reg) {
   uint8_t buf;
   i2c_read(&g_pmic_i2c_dev_reg, reg, &buf, 1);
   return buf;
} 

static void set_pmic_reg(uint8_t reg, uint8_t value) {
   i2c_write(&g_pmic_i2c_dev_reg, reg, &value ,1);
}

// ***********************************
// Specific Register Access
// ***********************************

uint8_t pmic_read_soc_power(void) {
  return(get_pmic_reg(SOC_POWER));
}

void pmic_set_temperature_threshold(uint8_t reg, uint8_t limit) {
   set_pmic_reg(reg,limit);
}

void pmic_set_tdp_threshold(uint8_t limit) {
   set_pmic_reg(SYSTEM_TDP,limit);
}

uint8_t pmic_get_temperature(void) {
   return(get_pmic_reg(SOC_TEMPERATURE));
}

uint8_t pmic_get_voltage(uint8_t shire) {
   return(get_pmic_reg(shire));
}

