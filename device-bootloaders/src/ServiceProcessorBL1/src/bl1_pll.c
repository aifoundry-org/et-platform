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

#include <stdio.h>
#include "etsoc/isa/io.h"
#include "printx.h"
#include "bl1_main.h"
#include "bl1_pll.h"
#include "hpdpll_register_defines.h"

#include "hwinc/hal_device.h"

SP_PLL_STATE_t get_pll_requested_percent(void)
{
    const SERVICE_PROCESSOR_BL1_DATA_t *bl1_data = get_service_processor_bl1_data();

    switch ((bl1_data->sp_gpio_pins >> 3) & 0x3)
    {
        case 0:
            return SP_PLL_STATE_100_PER_CENT; // all PLLs are configured to 100%
        case 1:
            return SP_PLL_STATE_75_PER_CENT; // all PLLs are configured to 75%
        case 2:
            return SP_PLL_STATE_50_PER_CENT; // all PLLs are configured to 50%
        default:
            return SP_PLL_STATE_OFF; // all PLLs are left OFF
    }
}

void check_pll_otp_override_values(void)
{
    const uint32_t *pll_registers;
    uint32_t reg_0x2;
    uint32_t reg_0x3;
    uint32_t reg_0x18;

    /* Check PLL0 OTP override values */
    pll_registers = (uint32_t *)R_SP_PLL0_BASEADDR;
    reg_0x2 = pll_registers[PLL_REG_INDEX_REG_FCW_INT];
    reg_0x3 = pll_registers[PLL_REG_INDEX_REG_FCW_FRAC];
    reg_0x18 = pll_registers[PLL_REG_INDEX_REG_LDO_CONTROL];
    if (reg_0x2 != 0x13 && get_pll_requested_percent() == SP_PLL_STATE_100_PER_CENT)
        printx("WARNING: PLL0 reg 0x2 has wrong OTP override value! Expected: %x Actual: %x\r\n",
               0x13, reg_0x2);
    if (reg_0x2 != 0xE && (get_pll_requested_percent() == SP_PLL_STATE_50_PER_CENT ||
                           get_pll_requested_percent() == SP_PLL_STATE_75_PER_CENT))
        printx("WARNING: PLL0 reg 0x2 has wrong OTP override value! Expected: %x Actual: %x\r\n",
               0xE, reg_0x2);
    if (reg_0x3 != 0xE666 && get_pll_requested_percent() != SP_PLL_STATE_OFF)
        printx("WARNING: PLL0 reg 0x3 has wrong OTP override value! Expected: %x Actual: %x\r\n",
               0xE666, reg_0x3);
    if (reg_0x18 != 0x35 && get_pll_requested_percent() != SP_PLL_STATE_OFF)
        printx("WARNING: PLL0 reg 0x18 has wrong OTP override value! Expected: %x Actual: %x\r\n",
               0x35, reg_0x18);

    /* Check PLL1 OTP override values */
    pll_registers = (uint32_t *)R_SP_PLL1_BASEADDR;
    reg_0x18 = pll_registers[PLL_REG_INDEX_REG_LDO_CONTROL];
    if (reg_0x18 != 0x15 && get_pll_requested_percent() == SP_PLL_STATE_100_PER_CENT)
        printx("WARNING: PLL1 reg 0x18 has wrong OTP override value! Expected: %x Actual: %x\r\n",
               0x15, reg_0x18);
    if (reg_0x18 != 0x35 && (get_pll_requested_percent() == SP_PLL_STATE_50_PER_CENT ||
                             get_pll_requested_percent() == SP_PLL_STATE_75_PER_CENT))
        printx("WARNING: PLL1 reg 0x18 has wrong OTP override value! Expected: %x Actual: %x\r\n",
               0x35, reg_0x18);

    /* Check PCIE PLL OTP override values */
    pll_registers = (uint32_t *)R_PCIE_PLLP0_BASEADDR;
    reg_0x18 = pll_registers[PLL_REG_INDEX_REG_LDO_CONTROL];
    if (reg_0x18 != 0x35 && get_pll_requested_percent() != SP_PLL_STATE_OFF)
        printx(
            "WARNING: PCIE PLL reg 0x18 has wrong OTP override value! Expected: %x Actual: %x\r\n",
            0x35, reg_0x18);
}