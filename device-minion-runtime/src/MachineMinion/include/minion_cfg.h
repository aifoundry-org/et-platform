
/***********************************************************************
*
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
************************************************************************/
/*! \file minion_cfg.h
    \brief A C header that define configuration functions for Compute Minions.
*/
/***********************************************************************/

#include <stdint.h>

// Configure Minion :
//    Program PLL to mode - all Minion programmed to same mode
//    Enable Minion Cores - Pull Minion Cores out of Reset
int64_t configure_compute_minion(uint64_t shire_mask, uint64_t pll_mode);

// Update Minion PLL frequency - all Minion programmed to same frequency
int64_t dynamic_minion_pll_frequency_update(uint64_t freq);
