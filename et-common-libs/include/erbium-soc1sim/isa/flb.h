/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

/* Fast Local Barrier (FLB) services for the erbium-soc1sim backend.
 * Public API matches the native-erbium side exactly. */

#ifndef _ERBIUM_SOC1SIM_ISA_FLB_H_
#define _ERBIUM_SOC1SIM_ISA_FLB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "erbium/isa/esr_defines.h"
#include "hwinc/etsoc_shire_other_esr.h"   /* ETSOC_SHIRE_OTHER_ESR_FAST_LOCAL_BARRIER0_BYTE_ADDRESS */

/* Number of Fast Local Barriers. Matches the CSR's 5-bit barrier
 * index in WAIT_FLB and the FAST_LOCAL_BARRIER0..31 ESRs. */
#define FLB_COUNT 32

/* Reset the given FLB to 0 by writing its memory-mapped ESR. */
#define INIT_FLB(barrier)                                                          \
    esr_write_u64(PRV_U, ESR_SR_CPU,                                               \
                  ETSOC_SHIRE_OTHER_ESR_FAST_LOCAL_BARRIER0_BYTE_ADDRESS           \
                      + (barrier) * (uint32_t)sizeof(uint64_t),                    \
                  0U)

/* Read the current FLB counter via its memory-mapped ESR. */
#define READ_FLB(barrier)                                                          \
    esr_read_u64(PRV_U, ESR_SR_CPU,                                                \
                 ETSOC_SHIRE_OTHER_ESR_FAST_LOCAL_BARRIER0_BYTE_ADDRESS            \
                     + (barrier) * (uint32_t)sizeof(uint64_t))

/* Join FLB `barrier`; atomically increment its counter and compare
 * against `threads`. Sets `result` to 1 on the last arriver (counter
 * hit target and was reset to 0), 0 otherwise. */
#define WAIT_FLB(threads, barrier, result)                                 \
    do                                                                     \
    {                                                                      \
        const uint64_t val = (((threads) - 1U) << 5U) + (barrier);         \
        __asm__ __volatile__("csrrw %0, flb, %1" : "=r"(result) : "r"(val));\
    } while (0)

/* Function-form WAIT_FLB: returns 1 iff this hart was the last to
 * arrive. `match` is (thread_count - 1). */
static inline __attribute__((always_inline))
uint64_t flbarrier(uint64_t barrier_num, uint64_t match)
{
    uint64_t ret;
    uint64_t flb_arg = (match << 5) | (barrier_num & 0x1F);

    __asm__ __volatile__("csrrw %0, flb, %1" : "=r"(ret) : "r"(flb_arg));

    return ret;
}

/* Write a raw value into the FLB's memory-mapped ESR. */
static inline __attribute__((always_inline))
void flbarrier_set(uint32_t barrier_num, uint64_t value)
{
    esr_write_u64(PRV_U, ESR_SR_CPU,
                  ETSOC_SHIRE_OTHER_ESR_FAST_LOCAL_BARRIER0_BYTE_ADDRESS
                      + barrier_num * (uint32_t)sizeof(uint64_t),
                  value);
}

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_SOC1SIM_ISA_FLB_H_ */
