/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

/*
* Test: MRAM accessible after waking from deep sleep
*
* Boot code wakes MRAM. We put it back to sleep, then wake it again
* and verify that reads/writes work and bridge_status_reg shows
* mram_ready=0xF.
*
* Expected: PASS
*/

#include "test.h"
#include <stdint.h>

#define POWER_DOMAIN_REQ   ((volatile uint32_t *)0x02000038ull)
#define BRIDGE_STATUS_REG  ((volatile uint64_t *)0x02001008ull)
#define MRAM_DSLEEP_EN     (1u << 16)
#define MRAM_READY_MASK    0xF00ull

#define TEST_ADDR          ((volatile uint64_t *)0x40000200ull)
#define TEST_PATTERN       0xCAFEBABE12345678ull

int main() {
    /* Put MRAM to deep sleep */
    *POWER_DOMAIN_REQ = MRAM_DSLEEP_EN;

    /* Verify mram_ready is 0 while asleep */
    if ((*BRIDGE_STATUS_REG & MRAM_READY_MASK) != 0)
        TEST_FAIL;

    /* Wake MRAM */
    *POWER_DOMAIN_REQ = 0;

    /* Verify mram_ready shows all banks ready */
    if ((*BRIDGE_STATUS_REG & MRAM_READY_MASK) != MRAM_READY_MASK)
        TEST_FAIL;

    /* Verify MRAM read/write works */
    *TEST_ADDR = TEST_PATTERN;
    if (*TEST_ADDR != TEST_PATTERN)
        TEST_FAIL;

    TEST_PASS;
    return 0;
}
