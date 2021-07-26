#ifndef FLB_H
#define FLB_H

#include "device-common/esr_defines.h"

#include <stdint.h>

// shire = shire to send the credit to, 0-32 or 0xFF for "this shire"
// barrier = fast local barrier to use, 0-31
#define INIT_FLB(shire, barrier) \
    (*((volatile uint64_t *)ESR_SHIRE(shire, FAST_LOCAL_BARRIER0) + barrier) = 0U)

#define READ_FLB(shire, barrier) (*((volatile uint64_t *)ESR_SHIRE(shire, FAST_LOCAL_BARRIER0) + barrier)

#define WAIT_FLB(threads, barrier, result)                            \
    do {                                                              \
        const uint64_t val = ((threads - 1U) << 5U) + barrier;        \
        asm volatile("csrrw %0, flb0, %1" : "=r"(result) : "r"(val)); \
    } while (0)

#endif // FLB_H
