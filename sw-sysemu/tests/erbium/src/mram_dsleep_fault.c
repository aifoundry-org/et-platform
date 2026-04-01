/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

/*
* Test: MRAM access while in deep sleep triggers load access fault
*
* Boot code wakes MRAM by default, so we put it back to sleep
* by writing mram_dsleep_en=1 to PowerDomainReq, then attempt
* a load from MRAM.
*
* Expected: Load access fault (cause=5), test PASSes via trap handler
*/

#include "test.h"
#include "trap.h"
#include <stdint.h>

#define POWER_DOMAIN_REQ  ((volatile uint32_t *)0x02000038ull)
#define MRAM_DSLEEP_EN    (1u << 16)

int main() {
    /* Put MRAM back to deep sleep */
    *POWER_DOMAIN_REQ = MRAM_DSLEEP_EN;

    expect_exception(CAUSE_LOAD_ACCESS_FAULT);

    volatile uint64_t *mram = (volatile uint64_t *)0x40000200ull;
    uint64_t val = *mram;
    (void)val;

    TEST_FAIL;
    return 0;
}
