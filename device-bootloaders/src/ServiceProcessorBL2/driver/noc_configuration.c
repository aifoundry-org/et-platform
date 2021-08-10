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

#include "hal_noc_reconfig.h"
#include "slam_engine.h"

#include "noc_configuration.h"

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

   return SUCCESS;
}
