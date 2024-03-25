/************************************************************************
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
************************************************************************/
/*! \file maxion_configuration.c
    \brief A C module that implements the maxion shire configuration services.

    Public interfaces:
    Maxion_Shire_Channel_Enable
    Maxion_Set_BootVector
    Maxion_Init
    Maxion_Fill_Bootram_With_Nops
*/
/***********************************************************************/
#include "log.h"

#include "hwinc/etsoc_shire_other_esr.h"
#include "hwinc/etsoc_shire_cache_esr.h"
#include "hwinc/hal_device.h"
#include "hwinc/sp_cru_reset.h"
#include "hwinc/sp_misc.h"
#include "esr.h"

#include "bl2_sp_pll.h"
#include "bl2_pmic_controller.h"
#include "bl2_reset.h"
#include "thermal_pwr_mgmt.h"
#include "bl_error_code.h"
#include "bl2_flash_fs.h"
#include "etsoc/isa/io.h"

#include "delays.h"
#include <string.h>

#include "maxion_configuration.h"
#include "maxion_bootrom.h"

/*! \def SC_BANK_BROADCAST
    \brief Shire cache bank broadcast ID
*/
#ifndef SC_BANK_BROADCAST
#define SC_BANK_BROADCAST 0xFu
#endif

/*! \def SC_BANK_SIZE
    \brief Shire cache bank size (1MB)
*/
#ifndef SC_BANK_SIZE
#define SC_BANK_SIZE 0x400u
#endif

/*! \def SHIRE_ID_IOSHIRE
    \brief Shire ID for IOSHIRE (254)
*/
#ifndef SHIRE_ID_IOSHIRE
#define SHIRE_ID_IOSHIRE 0xFE
#endif

#ifndef SUB_REGION_ADDR
#define SUB_REGION_ADDR 0x02
#endif

#ifndef MASK_ALL_NEIGHBOURHOODS
#define MASK_ALL_NEIGHBOURHOODS 0xF
#endif

#define RISCV_NOP_INSTRUCTION_ENCODING (uint32_t)0x00000013

inline __attribute__((always_inline)) uint8_t get_highest_set_bit_offset(uint64_t shire_mask)
{
    return (uint8_t)(63 - __builtin_clzl(shire_mask));
}

void Maxion_Shire_Channel_Enable(void)
{
    /* Maxion Shire Channel enable*/

    volatile uint64_t *pEsr = esr_address_new(PP_MACHINE, SHIRE_ID_IOSHIRE, REGION_OTHER,
                                              SUB_REGION_ADDR,
                                              ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_ADDRESS, 0x0);

    *pEsr = ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_RESET_VALUE |
            ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_NEIGH_EN_SET(MASK_ALL_NEIGHBOURHOODS) |
            ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_CACHE_EN_SET(1) |
            ETSOC_SHIRE_OTHER_ESR_SHIRE_CONFIG_SHIRE_ID_SET(SHIRE_ID_IOSHIRE);
}

void Maxion_Shire_Cache_Partition(void)
{
    volatile uint64_t *pEsr;

    uint64_t scp_ctl_value;
    uint64_t l2_ctl_value;
    uint64_t l3_ctl_value;
    uint16_t set_mask;
    uint16_t tag_mask;
    uint8_t high_bit;

    /* Disable L3 and SCP */
    scp_ctl_value = ETSOC_SHIRE_CACHE_ESR_SC_SCP_CACHE_CTL_ESR_SC_SCP_SET_SIZE_SET(0);
    l3_ctl_value = ETSOC_SHIRE_CACHE_ESR_SC_L3_CACHE_CTL_ESR_SC_L3_SET_SIZE_SET(0);

    /* L2 calculation */
    high_bit = get_highest_set_bit_offset((uint64_t)SC_BANK_SIZE);
    set_mask = (uint16_t)((0x1u << high_bit) - 0x1u);
    tag_mask = ((0x1u << high_bit) == SC_BANK_SIZE) ?
                   (uint16_t)((0x1u << high_bit) - 0x1u) :
                   (uint16_t)((0x1u << (high_bit - 0x1u)) - 0x1u);

    l2_ctl_value = ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ESR_SC_L2_SET_BASE_SET(0) |
                   ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ESR_SC_L2_SET_SIZE_SET(SC_BANK_SIZE) |
                   ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ESR_SC_L2_SET_MASK_SET(set_mask) |
                   ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ESR_SC_L2_TAG_MASK_SET(tag_mask);

    /* Set scp cache ctrl */
    write_esr_new(PP_MACHINE, SHIRE_ID_IOSHIRE, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                  ETSOC_SHIRE_CACHE_ESR_SC_SCP_CACHE_CTL_ADDRESS, scp_ctl_value, SC_BANK_BROADCAST);

    /* Set l2 cache ctrl */
    write_esr_new(PP_MACHINE, SHIRE_ID_IOSHIRE, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                  ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ADDRESS, l2_ctl_value, SC_BANK_BROADCAST);

    /* Set l3 cache ctrl */
    write_esr_new(PP_MACHINE, SHIRE_ID_IOSHIRE, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                  ETSOC_SHIRE_CACHE_ESR_SC_L3_CACHE_CTL_ADDRESS, l3_ctl_value, SC_BANK_BROADCAST);

    /* Print out cache ctrl registers */
    pEsr = esr_address_new(PP_MACHINE, SHIRE_ID_IOSHIRE, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                           ETSOC_SHIRE_CACHE_ESR_SC_L2_CACHE_CTL_ADDRESS, 0x0);
    Log_Write(LOG_LEVEL_DEBUG, "\tSC_L2_CACHE_CTL: 0x%lx\r\n", *pEsr);
    pEsr = esr_address_new(PP_MACHINE, SHIRE_ID_IOSHIRE, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                           ETSOC_SHIRE_CACHE_ESR_SC_L3_CACHE_CTL_ADDRESS, 0x0);
    Log_Write(LOG_LEVEL_DEBUG, "\tSC_L3_CACHE_CTL: 0x%lx\r\n", *pEsr);
    pEsr = esr_address_new(PP_MACHINE, SHIRE_ID_IOSHIRE, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                           ETSOC_SHIRE_CACHE_ESR_SC_SCP_CACHE_CTL_ADDRESS, 0x0);
    Log_Write(LOG_LEVEL_DEBUG, "\tSC_SCP_CACHE_CTL: 0x%lx\r\n", *pEsr);
}

void Maxion_Set_BootVector(uint64_t address)
{
    iowrite32(R_SP_MISC_BASEADDR + SPIO_MISC_ESR_MAX_RESET_BOOT_VECTOR_LO_OFFSET,
              (uint32_t)(address & 0xFFFFFFFF));
    iowrite32(R_SP_MISC_BASEADDR + SPIO_MISC_ESR_MAX_RESET_BOOT_VECTOR_HI_OFFSET,
              (uint32_t)((address >> 32) & 0xFFFFFFFF));
}

void Maxion_Fill_Bootram_With_Nops(void)
{
    for (uint32_t *pInstr = (uint32_t *)R_MX_ROM_BASEADDR;
         pInstr < (uint32_t *)(R_MX_ROM_BASEADDR + R_MX_ROM_SIZE); pInstr++)
    {
        *pInstr = RISCV_NOP_INSTRUCTION_ENCODING;
    }
}

void Maxion_SetVoltage(uint8_t voltage)
{
    pmic_set_voltage(MODULE_MAXION, voltage);
    US_DELAY_GENERIC(5000)
    pmic_get_voltage(MODULE_MAXION, &voltage);
    Log_Write(LOG_LEVEL_INFO, "Overriding MAXION -> 850mV(0x%X)\n", voltage);
}

int Maxion_Configure_PLL_Core(uint8_t mode)
{
    return configure_maxion_pll_core(mode);
}

int Maxion_Configure_PLL_Uncore(uint8_t mode)
{
    return configure_maxion_pll_uncore(mode);
}

int Maxion_Init(uint8_t pllModeUncore, uint8_t pllModeCore)
{
    int status = 0;

    /* Setting the MAXION voltages */
    Maxion_SetVoltage(flash_fs_get_mxn_boot_voltage());

    Maxion_Reset_Cold_Release();

    Maxion_Reset_Warm_Uncore_Release();

    Maxion_Shire_Channel_Enable();

    Maxion_Shire_Cache_Partition();

    Maxion_Reset_PLL_Uncore_Release();

    Maxion_Reset_PLL_Core_Release();

    status = Maxion_Configure_PLL_Uncore(pllModeUncore);
    if (SUCCESS != status)
    {
        Log_Write(LOG_LEVEL_ERROR, "Error programming Maxion Uncore PLL\r\n");
        return status;
    }

    status = Maxion_Configure_PLL_Core(pllModeCore);
    if (SUCCESS != status)
    {
        Log_Write(LOG_LEVEL_ERROR, "Error programming Maxion Core PLL\r\n");
        return status;
    }

    return SUCCESS;
}

void Maxion_Initialize_Boot_Code(void)
{
    memcpy((void *)R_MX_ROM_BASEADDR, gs_maxion_bootrom_bin, gs_maxion_bootrom_bin_len);
}

void Maxion_Check_If_Booted(void)
{
    volatile uint32_t *pCheck = (uint32_t *)(R_PU_MBOX_MX_SP_BASEADDR);
    if (*pCheck == 0xDEADBEEF)
    {
        Log_Write(LOG_LEVEL_CRITICAL, "MAXION has booted!\r\n");
    }
    else
    {
        Log_Write(LOG_LEVEL_CRITICAL, "MAXION did not boot!\r\n");
    }
}
