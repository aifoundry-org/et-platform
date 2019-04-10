#ifndef SYNC_H
#define SYNC_H

#include <stdint.h>

#define THIS_SHIRE 0xFF
#define THREAD_0 0
#define THREAD_1 1
#define FCC_0 0
#define FCC_1 1

// Set 1 to DISABLE a thread 1: bit 0 = minion 0, bit 31 = minion 31
#define SET_THREAD1_DISABLE(bitmask) (*(volatile uint64_t* const)(0x100340010ULL) = bitmask)

// shire = shire to send the credit to, 0-32 or 0xFF for "this shire"
// thread = thread to send credit to, 0 or 1
// fcc = fast credit counter to send credit to, 0 or 1
// bitmask = which minion to send credit to. bit 0 = minion 0, bit 31 = minion 31
#define SEND_FCC(shire, thread, fcc, bitmask) (*(volatile uint64_t* const)(0x1003400C0ULL + (shire << 22) + (((thread * 2) + fcc) << 3)) = bitmask)

// fcc = fast credit counter to block on, 0 or 1
#define WAIT_FCC(fcc) asm volatile ("csrwi fcc, %0" : : "I" (fcc))

static inline void wait_fcc(unsigned int fcc)
{
    asm volatile ("csrw fcc, %0" : : "r" (fcc));
}

static inline unsigned int read_fcc(unsigned int fcc)
{
    unsigned int temp;
    unsigned int val;

    asm volatile (
        "      csrr  %0, fccnb  \n" // read FCCNB
        "      beqz  %2, mask   \n" // if FCC1, shift FCCNB 31:16 down to 15:0
        "      srli  %0, %0, 16 \n"
        "mask: lui   %1, 0x10   \n" // mask with 0xFFFF
        "      addiw %1, %1, -1 \n"
        "      and   %0, %0, %1 \n"
        : "=r" (val), "=&r" (temp)
        : "r" (fcc)
    );

    return val;
}

// shire = shire to send the credit to, 0-32 or 0xFF for "this shire"
// barrier = fast local barrier to use, 0-31
#define INIT_FLB(shire, barrier) (*(volatile uint64_t* const)(0x100340100ULL + (shire << 22) + (barrier << 3)) = 0U)

#define READ_FLB(shire, barrier) (*(volatile uint64_t* const)(0x100340100ULL + (shire << 22) + (barrier << 3)))

#define WAIT_FLB(threads, barrier, result) do \
{ \
    const uint64_t val = ((threads - 1U) << 5U) + barrier; \
    asm volatile ("csrrw %0, flb0, %1" : "=r" (result) : "r" (val)); \
} while (0)

#endif // SYNC_H
