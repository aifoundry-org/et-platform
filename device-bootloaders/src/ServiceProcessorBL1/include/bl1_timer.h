#ifndef __BL1_TIMER_H__
#define __BL1_TIMER_H__

#include <stdint.h>

void timer_init(void);
uint64_t timer_get_ticks_count(void);

#endif
