#ifndef FCC_H
#define FCC_H

#include "esr_defines.h"

#include <stdint.h>

#define THREAD_0   0
#define THREAD_1   1

typedef enum { FCC_0 = 0, FCC_1 = 1 } fcc_t;

// shire = shire to send the credit to, 0-32 or 0xFF for "this shire"
// thread = thread to send credit to, 0 or 1
// fcc = fast credit counter to send credit to, 0 or 1
// bitmask = which minion to send credit to. bit 0 = minion 0, bit 31 = minion 31
#define SEND_FCC(shire, thread, fcc, bitmask) \
    (*((volatile uint64_t *)ESR_SHIRE(shire, FCC_CREDINC_0) + (thread * 2) + fcc) = bitmask)

// fcc = fast credit counter to block on, 0 or 1
#define WAIT_FCC(fcc) asm volatile("csrwi fcc, %0" : : "I"(fcc))

static inline void wait_fcc(fcc_t fcc)
{
    asm volatile("csrw fcc, %0" : : "r"(fcc));
}

static inline uint64_t read_fcc(fcc_t fcc)
{
    uint64_t temp;
    uint64_t val;

    asm volatile("   csrr  %0, fccnb  \n" // read FCCNB
                 "   beqz  %2, 1f     \n" // if FCC1, shift FCCNB 31:16 down to 15:0
                 "   srli  %0, %0, 16 \n"
                 "1: lui   %1, 0x10   \n" // mask with 0xFFFF
                 "   addiw %1, %1, -1 \n"
                 "   and   %0, %0, %1 \n"
                 : "=&r"(val), "=r"(temp)
                 : "r"(fcc));

    return val;
}

static inline void init_fcc(fcc_t fcc)
{
    // Consume all credits
    for (uint64_t i = read_fcc(fcc); i > 0; i--) {
        wait_fcc(fcc);
    }
}

#endif // FCC_H
