/***********************************************************************/
/*! \copyright
  Copyright (c) 2026 Ainekko, Co.
  SPDX-License-Identifier: Apache-2.0
*/
/***********************************************************************/
/*! \file hart.h
    \brief Erbium hart-service API, backed by etsoc1 for simulation.
*/
/***********************************************************************/

#ifndef _ERBIUM_SOC1SIM_ISA_HART_H_
#define _ERBIUM_SOC1SIM_ISA_HART_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Keep the same erbium-visible constants; soc1sim presents a
   single-shire / single-neighborhood / 8-minion view even though the
   underlying hardware is a full etsoc1. */
#define SOC_MINIONS_PER_SHIRE 8

#define THIS_SHIRE 0

#include <stdint.h>

static inline unsigned int __attribute__((always_inline, const)) get_hart_id(void)
{
    uint64_t ret;

    __asm__ __volatile__("csrr %0, hartid" : "=r"(ret));

    /* Mask to local hart index within the shire (64 harts/shire). */
    return (unsigned int)(ret & 0x3F);
}

static inline unsigned int __attribute__((always_inline, const)) get_shire_id(void)
{
    return 0;
}

static inline unsigned int __attribute__((always_inline, const)) get_neighborhood_id(void)
{
    return 0;
}

static inline unsigned int __attribute__((always_inline, const)) get_minion_id(void)
{
    return get_hart_id() >> 1;
}

static inline unsigned int __attribute__((always_inline, const)) get_thread_id(void)
{
    return get_hart_id() & 1;
}

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_SOC1SIM_ISA_HART_H_ */
