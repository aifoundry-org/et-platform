
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

#include "esr_defines.h"
#include "broadcast.h"
#include "minion_cfg.h"


// Configure Minion PLL
// Configure Minion PLL to specific mode. This uses the broadcast mechanism hence all Minions 
// will be programmed to the same frequency. Details covered: https://esperantotech.atlassian.net/wiki/spaces/SW/pages/836337737/Power+Management+System+Software+Specification#Cold-Reset-Sequence
static int64_t minion_configure_pll(uint64_t shire_mask, uint64_t pll_mode)
{
    if (pll_mode == 10) 
        return -1;

    broadcast(0xb, shire_mask, PRV_M, ESR_SHIRE_REGION, ESR_SHIRE_SHIRE_CTRL_CLOCKMUX_REGNO);
 
   return 0;
}

// Enable Minion Core
// Enable all Minion  Cores within the active Shires
static int64_t enable_minion(uint64_t shire_mask)
{
    broadcast(0x0, shire_mask, PRV_M, ESR_SHIRE_REGION, ESR_SHIRE_THREAD0_DISABLE_REGNO);
    broadcast(0x0, shire_mask, PRV_M, ESR_SHIRE_REGION, ESR_SHIRE_THREAD1_DISABLE_REGNO);
    return 0;
}


int64_t configure_compute_minion(uint64_t shire_mask, uint64_t pll_mode )
{
    int64_t status;
	
    status = minion_configure_pll(shire_mask,pll_mode);
    if (status != 0) 
        return status;

    status = enable_minion(shire_mask);
	   
    return status;
}
