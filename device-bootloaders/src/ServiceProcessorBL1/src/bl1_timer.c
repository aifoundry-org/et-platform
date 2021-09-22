#include <stdint.h>
#include <stdbool.h>

#include "etsoc/isa/mem-access/io.h"
#include "bl1_timer.h"
#include "hwinc/sp_rvtim.h"
#include "hwinc/hal_device.h"
#include "bl1_main.h"

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
#define DIVIDER_100 10ULL // PLLs @ 100%
#define DIVIDER_75  8ULL // PLLs @ 75%
#define DIVIDER_50  5ULL // PLLs @ 50%
#define DIVIDER_OFF 4ULL // PLLs off

static uint32_t gs_timer_divider = DIVIDER_OFF;
static uint64_t gs_timer_raw_ticks_before_pll_turned_on = 0;
static uint64_t gs_timer_1_MHz_ticks_before_pll_turned_on = 0;

uint64_t timer_get_ticks_count(void)
{
    uint64_t tick_count;
    tick_count = ioread64(R_SP_RVTIM_BASEADDR + RVTIMER_MTIME_ADDRESS);
    if (0 == gs_timer_raw_ticks_before_pll_turned_on) {
        return tick_count / gs_timer_divider;
    } else {
        return gs_timer_1_MHz_ticks_before_pll_turned_on +
               ((tick_count - gs_timer_raw_ticks_before_pll_turned_on) / gs_timer_divider);
    }
}

void timer_init(void)
{
    SERVICE_PROCESSOR_BL1_DATA_t *bl1_data = get_service_processor_bl1_data();

    gs_timer_raw_ticks_before_pll_turned_on = bl1_data->timer_raw_ticks_before_pll_turned_on;
    gs_timer_1_MHz_ticks_before_pll_turned_on =
        gs_timer_raw_ticks_before_pll_turned_on / DIVIDER_OFF;

    switch (bl1_data->sp_pll0_frequency) {
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
