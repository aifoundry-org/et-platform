#ifndef FCC_H
#define FCC_H

#include "esr_defines.h"

#include <stdint.h>

#define THIS_SHIRE 0xFF
#define THREAD_0 0
#define THREAD_1 1
#define FCC_0 0
#define FCC_1 1

// shire = shire to send the credit to, 0-32 or 0xFF for "this shire"
// thread = thread to send credit to, 0 or 1
// fcc = fast credit counter to send credit to, 0 or 1
// bitmask = which minion to send credit to. bit 0 = minion 0, bit 31 = minion 31
#define SEND_FCC(shire, thread, fcc, bitmask) (*(ESR_SHIRE(0, shire, FCC0) + (thread * 2) + fcc) = bitmask)

// fcc = fast credit counter to block on, 0 or 1
#define WAIT_FCC(fcc) asm volatile ("csrwi fcc, %0" : : "I" (fcc))

static inline void wait_fcc(uint64_t fcc)
{
    asm volatile ("csrw fcc, %0" : : "r" (fcc));
}

static inline uint64_t read_fcc(uint64_t fcc)
{
    uint64_t temp;
    uint64_t val;

    asm volatile (
        "   csrr  %0, fccnb  \n" // read FCCNB
        "   beqz  %2, 1f     \n" // if FCC1, shift FCCNB 31:16 down to 15:0
        "   srli  %0, %0, 16 \n"
        "1: lui   %1, 0x10   \n" // mask with 0xFFFF
        "   addiw %1, %1, -1 \n"
        "   and   %0, %0, %1 \n"
        : "=r" (val), "=&r" (temp)
        : "r" (fcc)
    );

    return val;
}

#endif // FCC_H
