#pragma once

#include <stdint.h>

/* Section attributes */
#define CODE __attribute__((section(".text")))
#define DATA __attribute__((section(".data")))


/* System constants */
#define NUM_MINION_SHIRES 34
#define HARTS_PER_MINION  2
#define MINION_PER_NEIGH  8
#define HARTS_PER_NEIGH   (HARTS_PER_MINION * MINION_PER_NEIGH)
#define NEIGH_PER_SHIRE   4
#define MINIONS_PER_SHIRE (MINION_PER_NEIGH * NEIGH_PER_SHIRE)
#define HARTS_PER_SHIRE   (HARTS_PER_NEIGH * NEIGH_PER_SHIRE)


/* Other test helpers */
#define TRY_UNTIL(LABEL, TIMEOUT) \
    int LABEL = TIMEOUT;          \
    for (; LABEL > 0; --LABEL)

#define C_TEST_PASS asm volatile("fence; li a7,0x1feed000; csrw validation0,a7" ::: "a7")
#define C_TEST_FAIL asm volatile("fence; li a7,0x50bad000; csrw validation0,a7" ::: "a7")
#define FENCE       asm volatile("fence" ::: "memory")


static inline uint64_t get_hart_id(void)
{
    uint64_t rv;
    asm volatile("csrr %0,mhartid" : "=r"(rv)::);
    return rv;
}
