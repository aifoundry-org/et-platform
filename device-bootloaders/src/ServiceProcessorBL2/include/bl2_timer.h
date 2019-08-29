#ifndef __BL2_TIMER_H__
#define __BL2_TIMER_H__

#include <stdint.h>

void timer_init(uint64_t timer_raw_ticks_before_pll_turned_on, uint32_t sp_pll0_frequency);
uint64_t timer_get_ticks_count(void);

#endif
