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
#ifndef __BL2_SP_MEMSHIRE_PLL__
#define __BL2_SP_MEMSHIRE_PLL__

#include "etsoc_hal/inc/movellus_hpdpll_modes_config.h"

#define MEMSHIRE_PLL_CONFIG_BASE 0x61000000

#define DCO_NORMALIZATION_ENABLE__SHIFT 7u
#define DCO_NORMALIZATION_ENABLE__MASK  (1u << DCO_NORMALIZATION_ENABLE__SHIFT)

static void write_memshire_pll_reg(uint8_t ms_num, uint16_t reg_addr, uint16_t reg_data)
{
    uint64_t address = MEMSHIRE_PLL_CONFIG_BASE;
    volatile uint32_t *p;

    address |= (uint64_t)((ms_num << 26) | (reg_addr << 2));
    p = (uint32_t *)address;
    *p = reg_data;
}

static uint16_t read_memshire_pll_reg(uint8_t ms_num, uint16_t reg_addr)
{
    uint64_t address = MEMSHIRE_PLL_CONFIG_BASE;
    volatile uint32_t *p;

    address |= (uint64_t)((ms_num << 26) | (reg_addr << 2));
    p = (uint32_t *)address;
    return (uint16_t)*p;
}

static void update_memshire_pll_regs(uint8_t ms_num)
{
    write_memshire_pll_reg(ms_num, 0x38, 1);
}

static int program_memshire_pll(uint8_t ms_num, uint8_t mode)
{
    uint16_t lock;
    uint16_t reg0;

    /* Start programming PLL to mode N */
    if (!mode) 
    {
        return -1;
    } 
    else 
    {
        for (int i = 0; i < gs_hpdpll_settings[(uint32_t)(mode - 1)].count; i++) 
        {
            write_memshire_pll_reg(ms_num, gs_hpdpll_settings[(mode - 1)].offsets[i],
                                   gs_hpdpll_settings[(mode - 1)].values[i]);
        }
    }
    update_memshire_pll_regs(ms_num);

    /*dco_normalization toggle */
    reg0 = read_memshire_pll_reg(ms_num, 0x0);
    reg0 = reg0 & (uint16_t)~DCO_NORMALIZATION_ENABLE__MASK;
    write_memshire_pll_reg(ms_num, 0x0, reg0);
    update_memshire_pll_regs(ms_num);

    reg0 = read_memshire_pll_reg(ms_num, 0x0);
    reg0 = reg0 | (uint16_t)DCO_NORMALIZATION_ENABLE__MASK;
    write_memshire_pll_reg(ms_num, 0x0, reg0);
    update_memshire_pll_regs(ms_num);

    lock = 0;
    while (lock == 0) 
    {
        lock = read_memshire_pll_reg(ms_num, 0x39) & 1;
    }
    return 0;
}

static int configure_memshire_plls(void)
{
    if (0 != program_memshire_pll(0, 19))
        return -1;

    if (0 != program_memshire_pll(4, 19))
        return -1;

    return 0;
}

#endif