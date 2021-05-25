/************************************************************************
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
************************************************************************/
/*! \file minion_pll_dll.c.c
    \brief A C module that implements the minion PLL configuration services. It 
    provides functionality to enable/disbale minion PLLs.

    Public interfaces:
        configure_minion_plls_and_dlls
        enable_minion_neighborhoods
        enable_master_shire_threads
*/
/***********************************************************************/
#include <stdio.h>

#include <etsoc_hal/inc/etsoc_shire_other_esr.h>
#include <etsoc_hal/inc/sp_minion_cold_reset_sequence.h>

#include "bl2_minion_pll_and_dll.h"
#include "minion_esr_defines.h"

/*==================== Function Separator =============================*/

/*! \def TIMEOUT_PLL_CONFIG
    \brief PLL timeout configuration
*/
#define TIMEOUT_PLL_CONFIG 100000

/*! \def TIMEOUT_PLL_LOCK
    \brief lock timeout for PLL
*/
#define TIMEOUT_PLL_LOCK   100000

/*==================== Function Separator =============================*/
static void pll_config(uint8_t shire_id)
{
    uint64_t reg_value;

    /* Select 1 GHz from step_clock, Bits[2:0] = 3'b011. Bit 3 to '1' to go with DLL output */
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, 0xb);

    /* Auto-config register set dll_enable and get reset deasserted of the DLL */
    reg_value = SHIRE_OTHER_DLL_AUTO_CONFIG_PCLK_SEL |
                (0x1 << SHIRE_OTHER_DLL_AUTO_CONFIG_DLL_EN_OFF);
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_DLL_AUTO_CONFIG, reg_value);

    /* Wait until DLL is locked to change clock mux */
    while (!(read_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_DLL_READ_DATA) & 0x20000));
}

static void minion_pll_config(uint64_t shire_mask)
{
    for (uint8_t i = 0; i <= 32; i++) 
    {
        if (shire_mask & 1)
            pll_config(i);
        shire_mask >>= 1;
    }
}

int configure_minion_plls_and_dlls(uint64_t shire_mask)
{
    minion_pll_config(shire_mask);
    return 0;
}

int enable_minion_neighborhoods(uint64_t shire_mask)
{
    for (uint8_t i = 0; i <= 32; i++) 
    {
        if (shire_mask & 1) 
        {
            /* Set Shire ID, enable cache and all Neighborhoods */
            const uint64_t config = ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_SHIRE_ID_SET(i) |
                                    ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_CACHE_EN_SET(1) |
                                    ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_NEIGH_EN_SET(0xF);
            write_esr(PP_MACHINE, i, REGION_OTHER, SHIRE_OTHER_CONFIG, config);
        }
        shire_mask >>= 1;
    }
    return 0;
}

int enable_master_shire_threads(uint8_t mm_id)
{
    /* Enable only Device Runtime Management thread on Master Shire */
    write_esr(PP_MACHINE, mm_id, REGION_OTHER, SHIRE_OTHER_THREAD0_DISABLE, ~(MM_RT_THREADS));
    return 0;
}
