/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-----------------------------------------------------------------------*/

/* RISC-V mtime / mtimecmp access for etsoc. mtime/mtimecmp live in
 * the per-hart RVTIMER block (HART ESR sub-region);
 * mtime_local_target is per-shire and lives in the shire-level
 * (SHIRE_OTHER) ESR sub-region. M-mode required. */

#ifndef _ETSOC_DRIVERS_TIMER_H_
#define _ETSOC_DRIVERS_TIMER_H_

#include <stdint.h>

#include "etsoc/isa/esr_defines.h"
#include "etsoc/isa/hart.h"               /* THIS_SHIRE */
#include "hwinc/pu_rvtim.h"               /* RVTIMER_MTIME[_CMP]_BYTE_ADDRESS */
#include "hwinc/etsoc_shire_other_esr.h"  /* ETSOC_SHIRE_OTHER_ESR_MTIME_LOCAL_TARGET_BYTE_ADDRESS */

#ifdef __cplusplus
extern "C" {
#endif

static inline __attribute__((always_inline))
uint64_t timer_read_mtime(void)
{
    return esr_read_u64(PRV_M, THIS_SHIRE, ESR_SR_HART,
                        RVTIMER_MTIME_BYTE_ADDRESS);
}

static inline __attribute__((always_inline))
void timer_write_mtime(uint64_t val)
{
    esr_write_u64(PRV_M, THIS_SHIRE, ESR_SR_HART,
                  RVTIMER_MTIME_BYTE_ADDRESS, val);
}

static inline __attribute__((always_inline))
uint64_t timer_read_mtimecmp(void)
{
    return esr_read_u64(PRV_M, THIS_SHIRE, ESR_SR_HART,
                        RVTIMER_MTIMECMP_BYTE_ADDRESS);
}

static inline __attribute__((always_inline))
void timer_write_mtimecmp(uint64_t val)
{
    esr_write_u64(PRV_M, THIS_SHIRE, ESR_SR_HART,
                  RVTIMER_MTIMECMP_BYTE_ADDRESS, val);
}

static inline __attribute__((always_inline))
uint64_t timer_read_mtime_local_target(void)
{
    return esr_read_u64(PRV_M, THIS_SHIRE, ESR_SR_SHIRE,
                        ETSOC_SHIRE_OTHER_ESR_MTIME_LOCAL_TARGET_BYTE_ADDRESS);
}

static inline __attribute__((always_inline))
void timer_write_mtime_local_target(uint64_t val)
{
    esr_write_u64(PRV_M, THIS_SHIRE, ESR_SR_SHIRE,
                  ETSOC_SHIRE_OTHER_ESR_MTIME_LOCAL_TARGET_BYTE_ADDRESS, val);
}

#ifdef __cplusplus
}
#endif

#endif /* _ETSOC_DRIVERS_TIMER_H_ */
