/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

/*
* Test: Store to MRAM boot protocol region succeeds with warning
*
* The first 256 bytes of MRAM (0x40000000-0x400000FF) are reserved for
* the boot protocol.  Stores are allowed (no fault) but the emulator
* logs a warning.  Verify the store actually lands in memory.
*/

#include "test.h"
#include <stdint.h>

#define MRAM_BOOT_REGION 0x40000020ull  /* Payload SP offset */
#define TEST_PATTERN     0xDEADBEEFCAFEFEEDull

int main() {
    volatile uint64_t *boot_region = (volatile uint64_t *)MRAM_BOOT_REGION;

    *boot_region = TEST_PATTERN;

    if (*boot_region == TEST_PATTERN) {
        TEST_PASS;
    }

    TEST_FAIL;
    return 0;
}
