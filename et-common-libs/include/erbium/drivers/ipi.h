/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-----------------------------------------------------------------------*/

/* Inter-Processor Interrupt (IPI) trigger / trigger-clear access for
 * erbium. The IPI registers live in the per-CPU Machine_cpu ESR
 * sub-region, so M-mode is required. */

#ifndef _ERBIUM_DRIVERS_IPI_H_
#define _ERBIUM_DRIVERS_IPI_H_

#include <stdint.h>

#include "erbium/isa/esr_defines.h"
#include "erbium/isa/hart.h"   /* THIS_SHIRE */
#include "hwinc/esr.h"         /* MACHINE_CPU_IPI_TRIGGER_*_BYTE_OFFSET */

#ifdef __cplusplus
extern "C" {
#endif

static inline __attribute__((always_inline))
uint64_t ipi_read_trigger(void)
{
    return esr_read_u64(PRV_M, ESR_SR_CPU,
                        MACHINE_CPU_IPI_TRIGGER_BYTE_OFFSET);
}

static inline __attribute__((always_inline))
void ipi_write_trigger(uint64_t val)
{
    esr_write_u64(PRV_M, ESR_SR_CPU,
                  MACHINE_CPU_IPI_TRIGGER_BYTE_OFFSET, val);
}

static inline __attribute__((always_inline))
void ipi_write_trigger_clear(uint64_t val)
{
    esr_write_u64(PRV_M, ESR_SR_CPU,
                  MACHINE_CPU_IPI_TRIGGER_CLEAR_BYTE_OFFSET, val);
}

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_DRIVERS_IPI_H_ */
