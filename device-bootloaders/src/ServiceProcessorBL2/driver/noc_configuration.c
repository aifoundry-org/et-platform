/************************************************************************
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
************************************************************************/
/*! \file noc_configuration.c
    \brief A C module that implements the main NOC configuration services.

    Public interfaces:

        NOC_Configure
*/
/***********************************************************************/
#include <stdint.h>
#include "bl2_sp_pll.h"
#include "bl2_reset.h"

#include "bl2_sp_pll.h"
#include "bl_error_code.h"
#include "log.h"

#include "etsoc/isa/io.h"
#include "hwinc/sp_cru_reset.h"
#include "hwinc/hal_device.h"

#include "hwinc/ms_reg_def.h"
#include "hwinc/etsoc_shire_cache_esr.h"
#include "esr.h"

#include "hal_noc_reconfig.h"
#include "slam_engine.h"
#include "noc_configuration.h"
#include "hwinc/noc_esr.h"
#include "noc_reconfigure.h"

/* Globals for NOC Shire Remap */
static int g_displace = SPARE_SHIRE_BIT_POSITION;

static int getPhysicalID(int virtualID)
{
    // Validate virtualID to avoid array index out of bounds
    if (virtualID < 0 || virtualID > NUM_SHIRES - 1)
    {
        return -1;
    }
    else
    {
        return phy_id[virtualID];
    }
}

static uint64_t calculate_new_base_value(int new_virtual_id, bridge_t bridge,
                                         unsigned int bridge_id, bridge_range_t range)
{
    uint64_t value = 0UL;
    switch (range)
    {
        case BRIDGE_RANGE_SCP:
            value = BRIDGE_RANGE_SCP_ADBASE & ~VIRTUAL_ID_MASK_SCP;
            value |= (((uint64_t)new_virtual_id << 23) & VIRTUAL_ID_MASK_SCP);
            return value;
        case BRIDGE_RANGE_CSR:
            value = BRIDGE_RANGE_CSR_ADBASE & ~VIRTUAL_ID_MASK_CSR;
            value |= (((uint64_t)new_virtual_id << 22) & VIRTUAL_ID_MASK_CSR);
            return value;
        case BRIDGE_RANGE_DRAM:
            if (new_virtual_id == 32 || new_virtual_id == 33)
            {
                value = (new_virtual_id == 32) ? BRIDGE_RANGE_DRAM_ADBASE_VID_32 :
                                                 BRIDGE_RANGE_DRAM_ADBASE_VID_33;

                if (bridge == BRIDGE_NOC_ESR && bridge_id == 4)
                {
                    value &= ~0x80UL;
                }
                if ((bridge == BRIDGE_NOC_ESR && bridge_id == 5) ||
                    ((bridge == BRIDGE_NOC_ESR_BRIDGE_IOS || bridge == BRIDGE_NOC_ESR_BRIDGE_PS) &&
                     (bridge_id == 2)))
                {
                    value &= ~0x80UL;
                    value |= 0x40UL;
                }
                if ((bridge == BRIDGE_NOC_ESR_SIB_TOL3 && bridge_id == 1) ||
                    (bridge == BRIDGE_NOC_ESR_BRIDGE_IOS && bridge_id == 0) ||
                    (bridge == BRIDGE_NOC_ESR_BRIDGE_PS && bridge_id == 3))
                {
                    value |= 0x40UL;
                }
            }
            else
            {
                value = BRIDGE_RANGE_DRAM_ADBASE & ~VIRTUAL_ID_MASK_DRAM;
                value |= (((uint64_t)new_virtual_id << 6) & VIRTUAL_ID_MASK_DRAM);
            }
            if ((bridge == BRIDGE_NOC_ESR_BRIDGE_IOS || bridge == BRIDGE_NOC_ESR_BRIDGE_PS) &&
                (bridge_id == 2))
            {
                value |= 0x8UL;
            }
            return value;
        default:
            break;
    }
    return ~0UL;
}

static uint64_t calculate_new_mask_value(int new_virtual_id, bridge_t bridge,
                                         unsigned int bridge_id, bridge_range_t range)
{
    uint64_t value = 0UL;
    switch (range)
    {
        case BRIDGE_RANGE_SCP:
            return BRIDGE_RANGE_SCP_ADMASK;
        case BRIDGE_RANGE_CSR:
            value = BRIDGE_RANGE_CSR_ADMASK;
            if ((bridge == BRIDGE_NOC_ESR_BRIDGE_IOS && bridge_id == 6) ||
                (bridge == BRIDGE_NOC_ESR_BRIDGE_MEM && bridge_id == 0))
            {
                value |= 0x8UL;
            }
            return value;
        case BRIDGE_RANGE_DRAM:
            value = (new_virtual_id == 32) ?
                        BRIDGE_RANGE_DRAM_ADMASK_VID_32 :
                        (new_virtual_id == 33) ? BRIDGE_RANGE_DRAM_ADMASK_VID_33 :
                                                 BRIDGE_RANGE_DRAM_ADMASK;
            if ((bridge == BRIDGE_NOC_ESR_BRIDGE_IOS || bridge == BRIDGE_NOC_ESR_BRIDGE_PS) &&
                (bridge_id == 2))
            {
                value |= 0x8UL;
            }
            return value;
        default:
            break;
    }
    return ~0UL;
}

static void remap_regs(bridge_t bridge, unsigned int bridge_id, int old_virtual_id,
                       int new_virtual_id, long unsigned int offset)
{
    int old_phy_id = getPhysicalID(old_virtual_id);
    uint32_t(*adbase)[NUM_SHIRES];
    bridge_range_t range;

    switch (bridge)
    {
        case BRIDGE_NOC_ESR: {
            adbase = adbase_noc_esr_bridge;
            range = (bridge_id < NUM_NOC_ESR_BRIDGE_SCP) ? BRIDGE_RANGE_SCP : BRIDGE_RANGE_DRAM;
            break;
        }
        case BRIDGE_NOC_ESR_SIB_TOL3: {
            adbase = adbase_noc_esr_sib_tol3;
            range = (bridge_id < NUM_NOC_ESR_SIB_TOL3_SCP) ? BRIDGE_RANGE_SCP : BRIDGE_RANGE_DRAM;
            break;
        }
        case BRIDGE_NOC_ESR_SIB_TOSYS: {
            adbase = adbase_noc_esr_sib_tosys;
            range = BRIDGE_RANGE_CSR;
            break;
        }
        case BRIDGE_NOC_ESR_BRIDGE_IOS: {
            adbase = adbase_noc_esr_bridge_ios;
            range = (bridge_id < NUM_NOC_ESR_BRIDGE_IOS_DRAM) ?
                        BRIDGE_RANGE_DRAM :
                        (bridge_id < (NUM_NOC_ESR_BRIDGE_IOS_DRAM + NUM_NOC_ESR_BRIDGE_IOS_SCP) ?
                             BRIDGE_RANGE_SCP :
                             BRIDGE_RANGE_CSR);
            break;
        }
        case BRIDGE_NOC_ESR_BRIDGE_PS: {
            adbase = adbase_noc_esr_bridge_ps;
            range = (bridge_id < NUM_NOC_ESR_BRIDGE_PS_SCP) ?
                        BRIDGE_RANGE_SCP :
                        (bridge_id < (NUM_NOC_ESR_BRIDGE_PS_SCP + NUM_NOC_ESR_BRIDGE_PS_CSR) ?
                             BRIDGE_RANGE_CSR :
                             BRIDGE_RANGE_DRAM);
            break;
        }
        case BRIDGE_NOC_ESR_BRIDGE_MEM: {
            adbase = adbase_noc_esr_bridge_mem;
            range = BRIDGE_RANGE_CSR;
            break;
        }
        default: {
            break;
        }
    }

    if (old_phy_id != -1)
    {
        const uint64_t baseAddr = R_SP_MAIN_NOC_REGBUS_BASEADDR;
        uint64_t old_base = baseAddr + adbase[bridge_id][old_phy_id] + offset;
        uint64_t old_mask = baseAddr + adbase[bridge_id][old_phy_id] + offset + 8;

        *(uint64_t *)old_base = calculate_new_base_value(new_virtual_id, bridge, bridge_id, range);

        *(uint64_t *)old_mask = calculate_new_mask_value(new_virtual_id, bridge, bridge_id, range);
    }
    else
    {
        // Handle the case when virtual IDs are not found
        Log_Write(LOG_LEVEL_ERROR, "Error: Virtual ID not found.\n");
    }
}

static void remap_shire(int old_virtual_id, int new_virtual_id)
{
    for (unsigned int bridge_id = 0; bridge_id < NUM_NOC_ESR_BRIDGE; bridge_id++)
    {
        for (unsigned int min_shire = 0; min_shire < NOC_ESR_BRIDGE_ARRAY_COUNT; min_shire++)
        {
            remap_regs(BRIDGE_NOC_ESR, bridge_id, old_virtual_id, new_virtual_id,
                       min_shire * NOC_ESR_BRIDGE_ARRAY_ELEMENT_SIZE);
        }
    }

    for (unsigned int bridge_id = 0; bridge_id < NUM_NOC_ESR_SIB_TOL3; bridge_id++)
    {
        for (unsigned int min_shire = 0; min_shire < NOC_ESR_SIB_TOL3_ARRAY_COUNT; min_shire++)
        {
            remap_regs(BRIDGE_NOC_ESR_SIB_TOL3, bridge_id, old_virtual_id, new_virtual_id,
                       min_shire * NOC_ESR_SIB_TOL3_ARRAY_ELEMENT_SIZE);
        }
    }

    for (unsigned int bridge_id = 0; bridge_id < NUM_NOC_ESR_SIB_TOSYS; bridge_id++)
    {
        for (unsigned int min_shire = 0; min_shire < NOC_ESR_SIB_TOSYS_ARRAY_COUNT; min_shire++)
        {
            remap_regs(BRIDGE_NOC_ESR_SIB_TOSYS, bridge_id, old_virtual_id, new_virtual_id,
                       min_shire * NOC_ESR_SIB_TOSYS_ARRAY_ELEMENT_SIZE);
        }
    }

    for (unsigned int bridge_id = 0; bridge_id < NUM_NOC_ESR_BRIDGE_IOS; bridge_id++)
    {
        remap_regs(BRIDGE_NOC_ESR_BRIDGE_IOS, bridge_id, old_virtual_id, new_virtual_id, 0);
    }

    for (unsigned int bridge_id = 0; bridge_id < NUM_NOC_ESR_BRIDGE_PS; bridge_id++)
    {
        remap_regs(BRIDGE_NOC_ESR_BRIDGE_PS, bridge_id, old_virtual_id, new_virtual_id, 0);
    }

    for (unsigned int bridge_id = 0; bridge_id < NUM_NOC_ESR_BRIDGE_MEM; bridge_id++)
    {
        for (unsigned int mem_shire = 0; mem_shire < NOC_ESR_BRIDGE_MEM_ARRAY_COUNT; mem_shire++)
        {
            remap_regs(BRIDGE_NOC_ESR_BRIDGE_MEM, bridge_id, old_virtual_id, new_virtual_id,
                       mem_shire * NOC_ESR_BRIDGE_MEM_ARRAY_ELEMENT_SIZE);
        }
    }
}

static void reconfig_memory_shire(CSR_SLAM_TABLE *reconfig_table_ptr)
{
    if (reconfig_table_ptr == CSR_SLAM_TABLE_PTR_NULL)
        return;

    SLAM_ENGINE(reconfig_table_ptr);

    Log_Write(LOG_LEVEL_INFO, "Re-configure NOC for the number of memory shire\n");
    // sequence from https://esperantotech.atlassian.net/browse/VERIF-3949
    // release memshire from reset
    release_memshire_from_reset();

    // configure the memshire control ms_mem_ctl register
    for (uint8_t i = 0; i < 8; i++)
    {
        volatile uint64_t *pRegs64 = (volatile uint64_t *)(ms_mem_ctl | ((232 + i) << 22));
        if (reconfig_table_ptr == &reconfig_noc_2_west_memshires ||
            reconfig_table_ptr == &reconfig_noc_2_east_memshires)
        {
            pRegs64[0] = 0x000307f187fff1c6;
        }
        else if (reconfig_table_ptr == &reconfig_noc_1_west_memshires ||
                 reconfig_table_ptr == &reconfig_noc_1_east_memshires)
        {
            pRegs64[0] = 0x000307f186ffffc6;
        }
        else if (reconfig_table_ptr == &reconfig_noc_4_west_memshires ||
                 reconfig_table_ptr == &reconfig_noc_4_east_memshires)
        {
            pRegs64[0] = 0x000307f188fc81c6;
        }
    }
}

static void reconfig_minion_shire(CSR_SLAM_TABLE *reconfig_table_ptr)
{
    if (reconfig_table_ptr == CSR_SLAM_TABLE_PTR_NULL)
        return;

    Log_Write(LOG_LEVEL_INFO, "Re-configure NOC for the number of minion shires\n");

    // sequence from https://esperantotech.atlassian.net/browse/VERIF-3950
    // needed only for 8 shires or less
    if (reconfig_table_ptr != &reconfig_noc_16_minshires)
    {
        for (uint8_t i = 0; i < 16; i++)
        {
            for (uint8_t j = 0; j < 4; j++)
            {
                //                                   | PP = 0x3, | shireid |                    | BBBB (bank)     | sc_l3_shire_swizzle_ctl reg addr = 0x0000
                // pRegs64 = (uint64_t)((0x100000000 | 0x3 << 30 | (i & 0x7f) << 22 | 0x3 << 20 | (j & 0xf) << 13 | 0x0000 << 3) & 0x1ffffffff)
                // pRegs64[0] |= 0x1000000000000
                uint64_t value =
                    read_esr_new(PP_MACHINE, i, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                                 ETSOC_SHIRE_CACHE_ESR_SC_L3_SHIRE_SWIZZLE_CTL_ADDRESS, j);
                value =
                    ETSOC_SHIRE_CACHE_ESR_SC_L3_SHIRE_SWIZZLE_CTL_ESR_SC_ALL_SHIRE_ALIASING_MODIFY(
                        value, 0x1ULL);
                write_esr_new(PP_MACHINE, i, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE,
                              ETSOC_SHIRE_CACHE_ESR_SC_L3_SHIRE_SWIZZLE_CTL_ADDRESS, value, j);
            }
        }
    }

    SLAM_ENGINE(reconfig_table_ptr);
}

int32_t NOC_Configure(uint8_t mode)
{
    if (0 != configure_sp_pll_2(mode, HPDPLL_LDO_KICK))
    {
        Log_Write(LOG_LEVEL_ERROR, "configure_sp_pll_2() failed!\n");
        return NOC_MAIN_CLOCK_CONFIGURE_ERROR;
    }

    /* tables are avaiable in hal_noc_reconfig.h.  Currently we have
        reconfig_noc_4_west_memshires
        reconfig_noc_2_west_memshires
        reconfig_noc_1_west_memshires
        reconfig_noc_4_east_memshires
        reconfig_noc_2_east_memshires
        reconfig_noc_1_east_memshires
    */
    reconfig_memory_shire(CSR_SLAM_TABLE_PTR_NULL);

    // Option to reconfigure NOC for different # of MinShires
    /* tables are avaiable in hal_noc_reconfig.h.  Currently we have
        reconfig_noc_16_minshires
        reconfig_noc_8_minshires
        reconfig_noc_4_minshires
        reconfig_noc_2_minshires
        reconfig_noc_1_minshires
    */
    reconfig_minion_shire(CSR_SLAM_TABLE_PTR_NULL);

    return SUCCESS;
}

int32_t Set_Displace_Shire_Id(uint64_t shire_mask)
{
    if (shire_mask == SHIRE_MASK_DEFAULT)
    {
        // All is good, no remap required
        Log_Write(LOG_LEVEL_INFO, "Using the default shire mask, no shire to be displaced.\n");
        return SUCCESS;
    }

    // Unless we're using the default mask, spare shire bit must be set and
    // there can be only one disabled shire.
    if ((HIGHEST_SET_BIT_POSITION(shire_mask) != SPARE_SHIRE_BIT_POSITION) ||
        (NUM_ENABLED_SHIRES(shire_mask) != (NUM_SHIRES - 1)))
    {
        Log_Write(LOG_LEVEL_ERROR, "Error: Invalid shire mask\n");
        return ERROR_INVALID_ARGUMENT;
    }

    g_displace = DISPLACE_SHIRE(shire_mask);

    if ((g_displace < 0) || (g_displace > MAX_SHIRE_BIT_POSITION))
    {
        return ERROR_INVALID_ARGUMENT;
    }

    return SUCCESS;
}

int32_t Get_Displace_Shire_Id(void)
{
    return g_displace;
}

int32_t Get_Spare_Shire_Id(void)
{
    return SPARE_SHIRE_BIT_POSITION;
}

int Get_New_Virtual_Id(int shire_id)
{
    return new_virtual_map[Get_Displace_Shire_Id()][shire_id];
}

int32_t NOC_Remap_Shires(void)
{
    for (int id = 0; id < NUM_SHIRES; id++)
    {
        remap_shire(id, Get_New_Virtual_Id(id));
    }

    return SUCCESS;
}
