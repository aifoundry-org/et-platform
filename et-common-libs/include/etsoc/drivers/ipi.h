/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-----------------------------------------------------------------------*/

/* Inter-Processor Interrupt (IPI) trigger / trigger-clear access for
 * etsoc. The IPI registers live in the shire-level (SHIRE_OTHER) ESR
 * sub-region; M-mode required. */

#ifndef _ETSOC_DRIVERS_IPI_H_
#define _ETSOC_DRIVERS_IPI_H_

#include <stdint.h>

#include "etsoc/isa/esr_defines.h"
#include "etsoc/isa/hart.h"               /* THIS_SHIRE */
#include "hwinc/etsoc_shire_other_esr.h"  /* ETSOC_SHIRE_OTHER_ESR_IPI_TRIGGER_*_ADDRESS */

#ifdef __cplusplus
extern "C" {
#endif

static inline __attribute__((always_inline))
uint64_t ipi_read_trigger(void)
{
    return esr_read_u64(PRV_M, THIS_SHIRE, ESR_SR_SHIRE,
                        ETSOC_SHIRE_OTHER_ESR_IPI_TRIGGER_BYTE_ADDRESS);
}

static inline __attribute__((always_inline))
void ipi_write_trigger(uint64_t val)
{
    esr_write_u64(PRV_M, THIS_SHIRE, ESR_SR_SHIRE,
                  ETSOC_SHIRE_OTHER_ESR_IPI_TRIGGER_BYTE_ADDRESS, val);
}

static inline __attribute__((always_inline))
void ipi_write_trigger_clear(uint64_t val)
{
    esr_write_u64(PRV_M, THIS_SHIRE, ESR_SR_SHIRE,
                  ETSOC_SHIRE_OTHER_ESR_IPI_TRIGGER_CLEAR_BYTE_ADDRESS, val);
}

#ifdef __cplusplus
}
#endif

#endif /* _ETSOC_DRIVERS_IPI_H_ */
