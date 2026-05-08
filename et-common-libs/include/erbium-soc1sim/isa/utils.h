/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef _ERBIUM_ISA_UTILS_H_
#define _ERBIUM_ISA_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * General
 */

#define ALIGN(x, a) (((x) + ((a)-1)) & ~((a)-1))

/*
 * Cache
 */

#define CACHE_LINE_SIZE 64

/* Define a structure that is both cache-line aligned and padded up
 * to a whole number of cache lines, so independent instances don't
 * share a line (no false sharing between atomics). */
#define CACHE_STRUCT(_f)                                                      \
    union {                                                                   \
        struct _f;                                                            \
        char pad[((sizeof(struct _f) + CACHE_LINE_SIZE - 1) /                 \
                  CACHE_LINE_SIZE * CACHE_LINE_SIZE)];                        \
    } __attribute__((aligned(CACHE_LINE_SIZE)))

/*
 * RISC-V
 */

#define NOP   __asm__ __volatile__("nop\n")
#define FENCE __asm__ __volatile__("fence\n")

/* tensor_wait.6 — stall until outstanding cache-op CSR writes
 * (evict_va / flush_va / prefetch_va) have retired. Needed before
 * downstream code can assume the evict/flush has taken effect. */
#define WAIT_CACHEOPS __asm__ __volatile__("csrwi tensor_wait, 6\n" : :)

#define WFI __asm__ __volatile__("wfi\n")

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_ISA_UTILS_H_ */
