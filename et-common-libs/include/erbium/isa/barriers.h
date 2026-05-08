/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

/* Shire-scope barrier for erbium.
 */

#ifndef _ERBIUM_ISA_BARRIERS_H_
#define _ERBIUM_ISA_BARRIERS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "erbium/isa/esr_defines.h"
#include "erbium/isa/fcc.h"
#include "erbium/isa/flb.h"
#include "erbium/isa/hart.h"

/* shire_barrier — a shire-scope barrier composed of one FLB and one
 * FCC.
 *
 * `flb` and `fcc` name the FLB index and FCC id to reuse for this
 * barrier. `thread_count` is the total number of harts expected to
 * hit the barrier; `minion_mask_t0`/`_t1` select the thread-0 and
 * thread-1 recipients of the wake-up credit.
 *
 * Returns non-zero for the one hart that was last to arrive at the
 * FLB; that hart is responsible for broadcasting the FCC credit
 * that releases the others. Every hart then consumes one credit
 * from the selected FCC.
 */
static inline __attribute__((always_inline))
uint64_t shire_barrier(uint64_t flb, uint64_t fcc,
                       uint64_t thread_count,
                       uint64_t minion_mask_t0, uint64_t minion_mask_t1)
{
    uint64_t last = flbarrier(flb, thread_count - 1);

    if (last)
    {
        fcc_send(THREAD_0, (uint32_t)fcc, minion_mask_t0);
        fcc_send(THREAD_1, (uint32_t)fcc, minion_mask_t1);
    }
    fcc_consume(fcc);

    return last;
}

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_ISA_BARRIERS_H_ */
