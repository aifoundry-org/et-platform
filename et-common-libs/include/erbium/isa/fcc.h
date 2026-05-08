/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

/* Fast Credit Counter (FCC) services for erbium.
 *
 */

#ifndef _ERBIUM_ISA_FCC_H_
#define _ERBIUM_ISA_FCC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "erbium/isa/esr_defines.h"
#include "hwinc/esr.h"   /* USER_CPU_CREDINC0_BYTE_OFFSET */

/* FCC counters available per hart. */
typedef enum { FCC_0 = 0, FCC_1 = 1 } fcc_t;

/* \def SEND_FCC(thread, fcc, bitmask)
 * Write a credit to (thread, fcc) for each minion selected in `bitmask`.
 * Targets the caller's own shire (THIS_SHIRE).
 *
 * The four CREDINC registers are laid out as a contiguous array of
 * uint64_t in the User_cpu ESR block:
 *     CREDINC_0 -> thread 0, fcc 0
 *     CREDINC_1 -> thread 0, fcc 1
 *     CREDINC_2 -> thread 1, fcc 0
 *     CREDINC_3 -> thread 1, fcc 1
 */
#define SEND_FCC(thread, fcc, bitmask)                                         \
    esr_write_u64(PRV_U, ESR_SR_CPU,                                           \
                  USER_CPU_CREDINC0_BYTE_OFFSET                                \
                      + ((thread) * 2 + (fcc)) * (uint32_t)sizeof(uint64_t),   \
                  (bitmask))

/* \def WAIT_FCC(fcc)
 * Attempt to decrement the hart's credit counter `fcc`. Stalls the
 * hart when the counter is 0 until a credit arrives. */
#define WAIT_FCC(fcc) __asm__ __volatile__("csrwi fcc, %0" : : "I"(fcc))

/* Reg-operand variant of WAIT_FCC. */
static inline __attribute__((always_inline)) void wait_fcc(fcc_t fcc)
{
    __asm__ __volatile__("csrw fcc, %0" : : "r"(fcc));
}

/* Read (and reset) the hart's FCC non-blocking counter via fccnb.
 * Returns the count for the selected FCC; FCC_0 is in bits [15:0],
 * FCC_1 in bits [31:16]. */
static inline __attribute__((always_inline)) uint64_t read_fcc(fcc_t fcc)
{
    uint64_t temp;
    uint64_t val;

    __asm__ __volatile__(
        "   csrr  %0, fccnb  \n"  /* read FCCNB */
        "   beqz  %2, 1f     \n"  /* FCC_0? stay with low 16 */
        "   srli  %0, %0, 16 \n"  /* FCC_1: shift [31:16] down */
        "1: lui   %1, 0x10   \n"  /* mask = 0xFFFF */
        "   addiw %1, %1, -1 \n"
        "   and   %0, %0, %1 \n"
        : "=&r"(val), "=r"(temp)
        : "r"(fcc));

    return val;
}

/* Drain any outstanding credits in the given FCC so it starts at 0. */
static inline __attribute__((always_inline)) void init_fcc(fcc_t fcc)
{
    for (uint64_t i = read_fcc(fcc); i > 0; i--)
    {
        wait_fcc(fcc);
    }
}

/* Function-form SEND_FCC (same write as the macro, expressed so the
 * compiler type-checks the arguments). */
static inline __attribute__((always_inline)) void fcc_send(
    uint32_t thread, uint32_t fcc_reg, uint64_t hart_mask)
{
    esr_write_u64(PRV_U, ESR_SR_CPU,
                  USER_CPU_CREDINC0_BYTE_OFFSET
                      + ((thread << 1) | fcc_reg) * (uint32_t)sizeof(uint64_t),
                  hart_mask);
}

/* Consume an arbitrary FCC id (0 or 1). Stalls like WAIT_FCC. */
static inline __attribute__((always_inline)) void fcc_consume(uint64_t fcc_reg)
{
    __asm__ __volatile__("csrw fcc, %0" : : "r"(fcc_reg));
}

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_ISA_FCC_H_ */
