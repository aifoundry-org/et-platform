/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-----------------------------------------------------------------------*/

/* Memory-protection (MPROT) register access for erbium.
 *
 * MPROT lives in the per-neighborhood Machine_neigh ESR sub-region;
 * M-mode required. Register holds a 9-bit configuration field; upper
 * bits are reserved.
 *
 * NOTE: hwinc/esr.h reports MACHINE_NEIGH_MPROT_BYTE_OFFSET = 0x38,
 * but the simulator (sw-sysemu/esrs_er.cpp) and the RTL place the
 * register at 0x20. Using the hardware-verified offset until the
 * RDL is corrected. */

#ifndef _ERBIUM_DRIVERS_MPROT_H_
#define _ERBIUM_DRIVERS_MPROT_H_

#include <stdint.h>

#include "erbium/isa/esr_defines.h"
#include "erbium/isa/hart.h"   /* THIS_SHIRE */

#ifdef __cplusplus
extern "C" {
#endif

#define MPROT_BYTE_OFFSET 0x20ul

static inline __attribute__((always_inline))
uint64_t mprot_read(void)
{
    return esr_read_u64(PRV_M, ESR_SR_NEIGH, MPROT_BYTE_OFFSET);
}

static inline __attribute__((always_inline))
void mprot_write(uint64_t val)
{
    esr_write_u64(PRV_M, ESR_SR_NEIGH, MPROT_BYTE_OFFSET, val);
}

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_DRIVERS_MPROT_H_ */
