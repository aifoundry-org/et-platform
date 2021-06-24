/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
************************************************************************/
/*! \file timer.c
    \brief A C module that implements timer sub system.

    Public interfaces:
        timer_init
        timer_get_ticks_count
*/
/***********************************************************************/
#include <stdint.h>
#include <stdbool.h>

#include "io.h"
#include "bl2_timer.h"

#include "hwinc/sp_rvtim.h"
#include "hwinc/hal_device.h"

/*
 * Initially the timer uses a 100 MHz oscillator clock.
 * After the SP PLL0 is locked and bypass disabled, the timer uses whatever is the output of the
 * PLL divided by 4, ex. 1 GHz / 4 = 250 MHz clock (when PLL is configured @ 100%)
 * In addition, the timer always uses a fixed divider value of 25.
 * So before the SP PLL0 is configured, locked and bypass disabled, the raw timer frequency is
 *   100 MHz / 25 = 4 MHz
 * After the SP PLL0 is configured, locked and bypass disabled, the raw timer frequenct is
 *   250 MHz / 25 = 10 MHz (@ 100%)
 *   187.5 MHz / 25 = 7.5 MHz (@ 100%)
 *   125 MHz / 25 = 5 MHz (@ 100%)
 *
 * SP assumes a timer tick frequency of 1 MHz, so the raw tick count will be divided by a constant
 * depending on the PLL frequency
 */

/*! \def DIVIDER_100
    \brief timer frequency divider define for 100% value
*/
#define DIVIDER_100 10ULL // PLLs @ 100%

/*! \def DIVIDER_75
    \brief timer frequency divider define for 75% value
*/
#define DIVIDER_75  8ULL // PLLs @ 75%

/*! \def DIVIDER_50
    \brief timer frequency divider define for 50% value
*/
#define DIVIDER_50  5ULL // PLLs @ 50%

/*! \def DIVIDER_OFF
    \brief timer divider off
*/
#define DIVIDER_OFF 4ULL // PLLs off

/*! \def RVTimer_INTERVAL
    \brief timer interval value
*/
#define RVTimer_INTERVAL 0x3FFFFFFULL

static uint32_t gs_timer_divider = DIVIDER_OFF;
static uint64_t gs_timer_raw_ticks_before_pll_turned_on = 0;
static uint64_t gs_timer_1_MHz_ticks_before_pll_turned_on = 0;

uint64_t timer_get_ticks_count(void)
{
    uint64_t tick_count;
    tick_count = ioread64(R_SP_RVTIM_BASEADDR + RVTIMER_MTIME_ADDRESS);
    if (0 == gs_timer_raw_ticks_before_pll_turned_on)
    {
        return tick_count / gs_timer_divider;
    }
    else
    {
        return gs_timer_1_MHz_ticks_before_pll_turned_on +
               ((tick_count - gs_timer_raw_ticks_before_pll_turned_on) / gs_timer_divider);
    }
}

void timer_init(uint64_t timer_raw_ticks_before_pll_turned_on, uint32_t sp_pll0_frequency)
{
    gs_timer_raw_ticks_before_pll_turned_on = timer_raw_ticks_before_pll_turned_on;
    gs_timer_1_MHz_ticks_before_pll_turned_on = timer_raw_ticks_before_pll_turned_on / DIVIDER_OFF;

    /* Setup RVTIMER interrupt intervals */
    iowrite64(R_SP_RVTIM_BASEADDR + RVTIMER_MTIMECMP_ADDRESS, RVTimer_INTERVAL);

    switch (sp_pll0_frequency)
    {
        case 1000:
            gs_timer_divider = DIVIDER_100;
            break;
        case 750:
            gs_timer_divider = DIVIDER_75;
            break;
        case 500:
            gs_timer_divider = DIVIDER_50;
            break;

        case 0:
        default:
            gs_timer_divider = DIVIDER_OFF;
            gs_timer_raw_ticks_before_pll_turned_on = 0;
            gs_timer_1_MHz_ticks_before_pll_turned_on = 0;
            break;
    }
}
