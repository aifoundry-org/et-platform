
/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
************************************************************************/
/*! \file minion_cfg.h
    \brief A C header that define configuration functions for Compute Minions.
*/
/***********************************************************************/

#include <stdint.h>

// Configure Minion :
//    Program PLL to mode - all Minion programmed to same mode
//    Enable Minion Cores - Pull Minion Cores out of Reset
int64_t configure_compute_minion(uint64_t shire_mask, uint64_t lvdpll_strap);

// Update Minion PLL frequency - all Minion programmed to same frequency
int64_t dynamic_minion_pll_frequency_update(uint64_t freq);

// Get PLL conf global
volatile struct pll_conf_reg_t *get_pll_conf_reg(void);

// Put the Neigh logic in reset
void disable_neigh(uint64_t shire_mask);

// Bring the Neigh logic out of reset
int64_t enable_neigh(uint64_t shire_mask);
