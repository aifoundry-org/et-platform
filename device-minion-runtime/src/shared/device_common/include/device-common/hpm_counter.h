#ifndef HPM_COUNTER_H
#define HPM_COUNTER_H

#include <stdint.h>

/* Ensure reads of hpmcounter3 don't have to go through the switch block */
static inline uint64_t hpm_read_counter3(void)
{
    uint64_t value = 0;
    __asm__ __volatile__("csrr %0, hpmcounter3\n" : "=r"(value));
    return value;
}

enum hpm_counter {
    HPM_COUNTER_4,
    HPM_COUNTER_5,
    HPM_COUNTER_6,
    HPM_COUNTER_7,
    HPM_COUNTER_8,
};

static inline uint64_t hpm_read_counter(enum hpm_counter cnt)
{
    uint64_t value = 0;
#define HPM_READ_COUNTER(C)                                                \
    case HPM_COUNTER_##C:                                                  \
        __asm__ __volatile__("csrr %0, hpmcounter" #C "\n" : "=r"(value)); \
        break;
    switch (cnt) {
        HPM_READ_COUNTER(4)
        HPM_READ_COUNTER(5)
        HPM_READ_COUNTER(6)
        HPM_READ_COUNTER(7)
        HPM_READ_COUNTER(8)
    default:
        break;
    }
#undef HPM_READ_COUNTER
    return value;
}

#endif
