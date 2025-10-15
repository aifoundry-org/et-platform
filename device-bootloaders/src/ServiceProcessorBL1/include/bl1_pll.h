/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

#ifndef __BL1_PLL_H__
#define __BL1_PLL_H__

/**
 * @enum SP_PLL_STATE_t
 * @brief Enum defining PLL states
 */
typedef enum SP_PLL_STATE_e
{
    SP_PLL_STATE_INVALID,
    SP_PLL_STATE_OFF,
    SP_PLL_STATE_50_PER_CENT,
    SP_PLL_STATE_75_PER_CENT,
    SP_PLL_STATE_100_PER_CENT
} SP_PLL_STATE_t;

SP_PLL_STATE_t get_pll_requested_percent(void);
void check_pll_otp_override_values(void);

#endif