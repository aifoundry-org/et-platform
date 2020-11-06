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

#include "etsoc_hal/inc/et_i2c.h"
#include "etsoc_hal/inc/DW_apb_i2c.h"

#include "bl2_pmic_controller.h"

// TODO: To be migrated to PMIC interface register... 
#define PMIC_SLAVE_ADDRESS 0x2

static ET_I2C_DEV_t g_pmic_i2c_dev_reg;

static void set_pmic_i2c_dev(void) 
{
    g_pmic_i2c_dev_reg.regs = (I2c*)R_SP_I2C0_BASEADDR;
    g_pmic_i2c_dev_reg.isInitialized = false;
}

void setup_pmic(void)
{
    set_pmic_i2c_dev();

    /*if (0 != i2c_init(&g_pmic_i2c_dev_reg, ET_I2C_SPEED_400k, PMIC_SLAVE_ADDRESS)) 
    {
     printf("PMIC connection failed to establish link\n");
    }*/

    printf("PMIC connection establish\n");
}

void pmic_toggle_etsoc_reset(void)
{
   printf("Reseting ETSOC \n");
   iowrite32(R_SP_CRU_BASEADDR + RESET_MANAGER_RM_SYS_RESET_CTRL_ADDRESS,
             RESET_MANAGER_RM_SYS_RESET_CTRL_ENABLE_SET(0x1));
}
