/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

/*
* Test: Enable all harts and verify they all execute
*
* Each hart writes its ID to MRAM[hartid * 8], then uses FLB (Fast Local
* Barrier) for synchronization. The last hart verifies all locations
* contain expected values.
*/

#include "test.h"
#include <stdint.h>

#define ESR_THREAD0_DISABLE       0x80F40240ULL
#define ESR_THREAD1_DISABLE       0x80F40010ULL
#define MRAM_BASE                 0x40000100ULL  /* past 256-byte boot protocol region */
#define NUM_HARTS                 16

#define CSR_FLB  0x820

/*
 * FLB (Fast Local Barrier):
 * - Returns 1 if this hart was the last (triggered wrap), 0 otherwise
 */
static inline int flb_barrier(unsigned barrier_id, unsigned num_harts) {
    uint64_t flb_val = barrier_id | ((num_harts - 1) << 5);
    uint64_t is_last;

    asm volatile("csrrw %0, %1, %2"
                 : "=r"(is_last)
                 : "i"(CSR_FLB), "r"(flb_val));

    return is_last;
}

int main() {
    volatile uint64_t *thread0_disable = (volatile uint64_t *)ESR_THREAD0_DISABLE;
    volatile uint64_t *thread1_disable = (volatile uint64_t *)ESR_THREAD1_DISABLE;
    volatile uint64_t *mram = (volatile uint64_t *)MRAM_BASE;

    uint64_t hartid = get_hart_id();

    if (hartid == 0) {
        *thread0_disable = 0x00;
        *thread1_disable = 0x00;
    }

    // Emulator is very predictable, let's introduce
    // some delay so barrier makes sense.
    if (hartid == 13) {
        for (int i = 0; i < 1000; i++) {
            asm volatile("nop");
        }
    }

    /* Each hart writes its ID to its slot */
    mram[hartid] = hartid + 1;  /* +1 so we can distinguish from unwritten (0) */

    /* Synchronize: wait for all harts to complete their writes */
    int last = flb_barrier(0, NUM_HARTS);

    /* Last hart to arrive verifies all values */
    if (last) {
        for (int i = 0; i < NUM_HARTS; i++) {
            if (mram[i] != (uint64_t)(i + 1)) {
                TEST_FAIL;
            }
        }
    }

    TEST_PASS;
    return 0;
}
