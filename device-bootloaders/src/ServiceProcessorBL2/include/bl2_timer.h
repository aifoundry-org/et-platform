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
/*! \file bl2_timer.h
    \brief A C header that defines the timer's public interfaces.
*/
/***********************************************************************/
#ifndef __BL2_TIMER_H__
#define __BL2_TIMER_H__

#include <stdint.h>

/*! \fn void timer_init(uint64_t timer_raw_ticks_before_pll_turned_on, uint32_t sp_pll0_frequency)
    \brief This function configures initializes timer subsystem
    \param timer_raw_ticks_before_pll_turned_on ticks value to wait before turing on PLL
    \param sp_pll0_frequency PLL0 frequency value
    \return none
*/
void timer_init(uint64_t timer_raw_ticks_before_pll_turned_on, uint32_t sp_pll0_frequency);

/*! \fn uint64_t timer_get_ticks_count(void)
    \brief This function returns ticks count value
    \param None
    \return ticks value
*/
uint64_t timer_get_ticks_count(void);

/*! \fn void timer_update()
    \brief This function updates timer SW dividers in order to match new PLL0 config
    \param sp_pll0_frequency PLL0 frequency value
    \return none
*/
void timer_update(uint32_t sp_pll0_frequency);

#endif
