/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-----------------------------------------------------------------------*/

/* Thread enable/disable control for etsoc. The two THREADn_DISABLE
 * registers live in the shire-level (SHIRE_OTHER) ESR sub-region;
 * M-mode required. */

#ifndef _ETSOC_DRIVERS_THREAD_H_
#define _ETSOC_DRIVERS_THREAD_H_

#include <stdint.h>

#include "etsoc/isa/esr_defines.h"
#include "etsoc/isa/hart.h"               /* THIS_SHIRE */
#include "hwinc/etsoc_shire_other_esr.h"  /* ETSOC_SHIRE_OTHER_ESR_THREAD[01]_DISABLE_*_ADDRESS */

#ifdef __cplusplus
extern "C" {
#endif

static inline __attribute__((always_inline))
void thread_write_thread0_disable(uint64_t val)
{
    esr_write_u64(PRV_M, THIS_SHIRE, ESR_SR_SHIRE,
                  ETSOC_SHIRE_OTHER_ESR_THREAD0_DISABLE_BYTE_ADDRESS, val);
}

static inline __attribute__((always_inline))
void thread_write_thread1_disable(uint64_t val)
{
    esr_write_u64(PRV_M, THIS_SHIRE, ESR_SR_SHIRE,
                  ETSOC_SHIRE_OTHER_ESR_THREAD1_DISABLE_BYTE_ADDRESS, val);
}

#ifdef __cplusplus
}
#endif

#endif /* _ETSOC_DRIVERS_THREAD_H_ */
