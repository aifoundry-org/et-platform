/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

/*
* Test: Verify threadX_disable ESR writes correctly disable harts
*
* This test exposes a bug where writing to thread0_disable or thread1_disable
* to disable harts causes iterator corruption in the emulator, resulting in:
* - Disabled harts continuing to execute
* - Enabled harts stopping unexpectedly
*
* Expected (bug fixed): Only H0 continues, signals PASS
* Actual (bug present): H1 or others continue, signal FAIL
*/

#include "test.h"
#include <stdint.h>

#define ESR_THREAD0_DISABLE       0x80F40240ULL
#define ESR_THREAD1_DISABLE       0x80F40010ULL
#define MRAM_BASE                 0x40000100ULL  /* past 256-byte boot protocol region */
#define MARKERS                   (MRAM_BASE)

int main() {
    volatile uint64_t *thread0_disable = (volatile uint64_t *)ESR_THREAD0_DISABLE;
    volatile uint64_t *thread1_disable = (volatile uint64_t *)ESR_THREAD1_DISABLE;
    volatile uint64_t *markers = (volatile uint64_t *)MARKERS;

    uint64_t hartid = get_hart_id();

    /* Phase 1: H0 initializes and enables all even harts */
    if (hartid == 0) {
        for (int i = 0; i < 16; i++) {
            markers[i] = 0;
        }
        *thread0_disable = 0x00;
        *thread1_disable = 0xFF;
    }

    /* Let all harts sync up */
    for (volatile int i = 0; i < 100; i++) {
        asm volatile("nop");
    }

    /*
     * Phase 2: All harts write 0xFE to thread0_disable
     * 0xFE: bit 0 = 0 (H0 enabled), bits 1-7 = 1 (H2-H14 disabled)
     */
    *thread0_disable = 0xFE;

    /* Give a moment for harts to get disabled (immediate on emulator) */
    for (volatile int i = 0; i < 200; i++) {
        asm volatile("nop");
    }

    /*
     * Phase 3: Mark presence - whoever runs writes their marker
     */
    markers[hartid] = hartid + 1;

    /*
     * Phase 4: Each hart checks if it should be running
     *
     * After writing 0xFE, only H0 should continue.
     * Any other hart reaching here means the bug is present.
     */
    if (hartid != 0) {
        /* BUG: This hart (H2, H4, etc.) should have been disabled! */
        TEST_FAIL;
        return 1;  /* Don't continue after signaling failure */
    }

    /* Double-check: only our marker should be set */
    for (int i = 2; i < 16; i += 2) {
        if (markers[i] != 0) {
            TEST_FAIL;  /* A disabled hart wrote! */
        }
    }

    TEST_PASS;
    return 0;
}
