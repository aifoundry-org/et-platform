/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file bl2_timer.h
    \brief A C header that defines the timer's public interfaces.
*/
/***********************************************************************/
#ifndef __BL2_TIMER_H__
#define __BL2_TIMER_H__

#include <stdint.h>
#include "FreeRTOS.h"

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

/*! \fn static inline uint64_t timer_convert_ticks_to_secs(uint64_t ticks)
    \brief This function converts timer ticks to seconds.
    \param ticks ticks value
    \return seconds value
*/
static inline uint64_t timer_convert_ticks_to_secs(uint64_t ticks)
{
    /* Still need to fine tune the timing of the clocks */
    return portTICK_RATE_MS * ticks / 1000 / 1000;
}

/*! \fn static inline uint64_t timer_convert_ticks_to_ms(uint64_t ticks)
    \brief This function converts timer ticks to milliseconds.
    \param ticks ticks value
    \return milliseconds value
*/
static inline uint64_t timer_convert_ticks_to_ms(uint64_t ticks)
{
    /* Still need to fine tune the timing of the clocks */
    return portTICK_RATE_MS * ticks / 1000;
}

#endif
