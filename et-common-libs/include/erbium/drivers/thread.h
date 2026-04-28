/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-----------------------------------------------------------------------*/

/* Thread enable/disable control for erbium. The two THREADn_DISABLE
 * registers live in the per-CPU Machine_cpu ESR sub-region; M-mode
 * is required. Writing a 1 disables the corresponding hart of the
 * caller's minion. */

#ifndef _ERBIUM_DRIVERS_THREAD_H_
#define _ERBIUM_DRIVERS_THREAD_H_

#include <stdint.h>

#include "erbium/isa/esr_defines.h"
#include "erbium/isa/hart.h"   /* THIS_SHIRE */
#include "hwinc/esr.h"         /* MACHINE_CPU_THREAD[01]_DISABLE_BYTE_OFFSET */

#ifdef __cplusplus
extern "C" {
#endif

static inline __attribute__((always_inline))
void thread_write_thread0_disable(uint64_t val)
{
    esr_write_u64(PRV_M, ESR_SR_CPU,
                  MACHINE_CPU_THREAD0_DISABLE_BYTE_OFFSET, val);
}

static inline __attribute__((always_inline))
void thread_write_thread1_disable(uint64_t val)
{
    esr_write_u64(PRV_M, ESR_SR_CPU,
                  MACHINE_CPU_THREAD1_DISABLE_BYTE_OFFSET, val);
}

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_DRIVERS_THREAD_H_ */
