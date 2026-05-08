/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-----------------------------------------------------------------------*/

/* RISC-V mtime / mtimecmp access for erbium. The timer registers
 * live in the per-CPU Machine_cpu ESR sub-region (M-mode required).
 * mtime_local_target is the per-hart local timer target. */

#ifndef _ERBIUM_DRIVERS_TIMER_H_
#define _ERBIUM_DRIVERS_TIMER_H_

#include <stdint.h>

#include "erbium/isa/esr_defines.h"
#include "erbium/isa/hart.h"   /* THIS_SHIRE */
#include "hwinc/esr.h"         /* MACHINE_CPU_MTIME[_CMP|_LOCAL_TARGET]_BYTE_OFFSET */

#ifdef __cplusplus
extern "C" {
#endif

static inline __attribute__((always_inline))
uint64_t timer_read_mtime(void)
{
    return esr_read_u64(PRV_M, ESR_SR_CPU,
                        MACHINE_CPU_MTIME_BYTE_OFFSET);
}

static inline __attribute__((always_inline))
void timer_write_mtime(uint64_t val)
{
    esr_write_u64(PRV_M, ESR_SR_CPU,
                  MACHINE_CPU_MTIME_BYTE_OFFSET, val);
}

static inline __attribute__((always_inline))
uint64_t timer_read_mtimecmp(void)
{
    return esr_read_u64(PRV_M, ESR_SR_CPU,
                        MACHINE_CPU_MTIME_CMP_BYTE_OFFSET);
}

static inline __attribute__((always_inline))
void timer_write_mtimecmp(uint64_t val)
{
    esr_write_u64(PRV_M, ESR_SR_CPU,
                  MACHINE_CPU_MTIME_CMP_BYTE_OFFSET, val);
}

static inline __attribute__((always_inline))
uint64_t timer_read_mtime_local_target(void)
{
    return esr_read_u64(PRV_M, ESR_SR_CPU,
                        MACHINE_CPU_MTIME_LOCAL_TARGET_BYTE_OFFSET);
}

static inline __attribute__((always_inline))
void timer_write_mtime_local_target(uint64_t val)
{
    esr_write_u64(PRV_M, ESR_SR_CPU,
                  MACHINE_CPU_MTIME_LOCAL_TARGET_BYTE_OFFSET, val);
}

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_DRIVERS_TIMER_H_ */
