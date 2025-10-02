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