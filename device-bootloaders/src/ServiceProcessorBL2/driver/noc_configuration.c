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

#include "bl_error_code.h"
#include "log.h"

#include "hwinc/etsoc_shire_cache_esr.h"
#include "esr.h"

#include "hal_noc_reconfig.h"
#include "slam_engine.h"

#include "noc_configuration.h"

static void reconfig_minion_shire(CSR_SLAM_TABLE* reconfig_table_ptr)
{
    if(reconfig_table_ptr == CSR_SLAM_TABLE_PTR_NULL)
        return;

    // sequence from https://esperantotech.atlassian.net/browse/VERIF-3950
    // needed only for 8 shires or less
    if(reconfig_table_ptr != &reconfig_noc_16_minshires) {
        for (uint8_t i = 0; i < 16; i++) {
            for (uint8_t j = 0 ; j < 4; j ++) {
                //                                   | PP = 0x3, | shireid |                    | BBBB (bank)     | sc_l3_shire_swizzle_ctl reg addr = 0x0000
                // pRegs64 = (uint64_t)((0x100000000 | 0x3 << 30 | (i & 0x7f) << 22 | 0x3 << 20 | (j & 0xf) << 13 | 0x0000 << 3) & 0x1ffffffff)
                // pRegs64[0] |= 0x1000000000000
                uint64_t value = read_esr_new(PP_MACHINE, i, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE, ETSOC_SHIRE_CACHE_ESR_SC_L3_SHIRE_SWIZZLE_CTL_ADDRESS, j);
                value = ETSOC_SHIRE_CACHE_ESR_SC_L3_SHIRE_SWIZZLE_CTL_ESR_SC_ALL_SHIRE_ALIASING_MODIFY(value, 0x1ull);
                write_esr_new(PP_MACHINE, i, REGION_OTHER, ESR_OTHER_SUBREGION_CACHE, ETSOC_SHIRE_CACHE_ESR_SC_L3_SHIRE_SWIZZLE_CTL_ADDRESS, value, j);
            }
        }
    }

    SLAM_ENGINE(reconfig_table_ptr);
}

int32_t NOC_Configure(uint8_t mode)
{
    if (0 != configure_sp_pll_2(mode)) {
        Log_Write(LOG_LEVEL_ERROR, "configure_sp_pll_2() failed!\n");
        return NOC_MAIN_CLOCK_CONFIGURE_ERROR;
    }

   // TBD: Configure Main NOC Registers via Regbus
   // Remap NOC ID based on MM OTP value
   // Other potential NOC workarounds

    /* tables are avaiable in hal_noc_reconfig.h.  Currently we have
        reconfig_noc_4_west_memshires
        reconfig_noc_2_west_memshires
        reconfig_noc_1_west_memshires
        reconfig_noc_4_east_memshires
        reconfig_noc_2_east_memshires
        reconfig_noc_1_east_memshires
    */
    CSR_SLAM_TABLE *reconfig_table_ptr = CSR_SLAM_TABLE_PTR_NULL;
    if(reconfig_table_ptr != CSR_SLAM_TABLE_PTR_NULL)
        SLAM_ENGINE(reconfig_table_ptr);

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
