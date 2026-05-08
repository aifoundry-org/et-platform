/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-----------------------------------------------------------------------*/

/* Memory-protection (MPROT) register access for etsoc. MPROT lives
 * in the per-neighborhood Machine_neigh ESR sub-region; M-mode
 * required. */

#ifndef _ETSOC_DRIVERS_MPROT_H_
#define _ETSOC_DRIVERS_MPROT_H_

#include <stdint.h>

#include "etsoc/isa/esr_defines.h"
#include "etsoc/isa/hart.h"           /* THIS_SHIRE */
#include "hwinc/etsoc_neigh_esr.h"    /* ETSOC_NEIGH_ESR_MPROT_BYTE_ADDRESS */

#ifdef __cplusplus
extern "C" {
#endif

static inline __attribute__((always_inline))
uint64_t mprot_read(void)
{
    return esr_read_u64(PRV_M, THIS_SHIRE, ESR_SR_NEIGH,
                        ETSOC_NEIGH_ESR_MPROT_BYTE_ADDRESS);
}

static inline __attribute__((always_inline))
void mprot_write(uint64_t val)
{
    esr_write_u64(PRV_M, THIS_SHIRE, ESR_SR_NEIGH,
                  ETSOC_NEIGH_ESR_MPROT_BYTE_ADDRESS, val);
}

#ifdef __cplusplus
}
#endif

#endif /* _ETSOC_DRIVERS_MPROT_H_ */
