/***********************************************************************/
/*! \copyright
  Copyright (c) 2025 Ainekko, Co.
  SPDX-License-Identifier: Apache-2.0
*/
/***********************************************************************/

/***********************************************************************/
/*! \file barriers.h
    \brief Header/Interface description for Barrier services
*/
/***********************************************************************/

#ifndef _BARRIERS_H_
#define _BARRIERS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#include "etsoc/isa/utils.h"
#include "etsoc/isa/esr_defines.h"

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

/*! \fn inline uint64_t shire_barrier(uint64_t flb, uint64_t fcc, uint64_t thread_count, uint64_t minion_mask_t0, uint64_t minion_mask_t1)
    \brief Shire-only barrier using FLBs and FCCs
    \param flb FLbarrier value
    \param fcc  FCC value
    \param thread_count active thread count
    \param minion_mask_t0 Mask of active thread0 minions
    \param minion_mask_t1 Mask of active thread1 minions
    \return last thread to reach barrier
    \syncops Implementation of shire_barrier api
*/
inline uint64_t __attribute__((always_inline)) shire_barrier(uint64_t flb, uint64_t fcc,
    uint64_t thread_count, uint64_t minion_mask_t0, uint64_t minion_mask_t1)
{
    uint64_t last = flbarrier(flb, thread_count - 1);

    if (last)
    {
        fcc_send(SHIRE_OWN, THREAD_0, fcc, minion_mask_t0);
        fcc_send(SHIRE_OWN, THREAD_1, fcc, minion_mask_t1);
    }
    fcc_consume(fcc);

    return last;
}

#ifdef __cplusplus
}
#endif

#endif
