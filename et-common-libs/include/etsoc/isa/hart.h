/***********************************************************************/
/*! \copyright
  Copyright (C) 2018 Esperanto Technologies Inc.
  The copyright to the computer program(s) herein is the
  property of Esperanto Technologies, Inc. All Rights Reserved.
  The program(s) may be used and/or copied only with
  the written permission of Esperanto Technologies and
  in accordance with the terms and conditions stipulated in the
  agreement/contract under which the program(s) have been supplied.
*/
/***********************************************************************/
/*! \file hart.h
    \brief A C header that defines the hart service available.
*/
/***********************************************************************/

#ifndef _ETSOC_ISA_HART_H_
#define _ETSOC_ISA_HART_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SOC_MINIONS_PER_SHIRE 32

#define THIS_SHIRE 0xFF

#include <stdint.h>

/*! \fn static inline unsigned int get_hart_id(void)
    \brief Get hart ID
    \return Hart ID
    \hartsvclib Click above for more info...
*/
static inline unsigned int __attribute__((always_inline, const)) get_hart_id(void)
{
    uint64_t ret;

    __asm__ __volatile__("csrr %0, hartid" : "=r"(ret));

    return (unsigned int)ret;
}

static inline unsigned int __attribute__((always_inline, const)) get_shire_id(void)
{
    return get_hart_id() >> 6;
}

static inline unsigned int __attribute__((always_inline, const)) get_neighborhood_id(void)
{
    return (get_hart_id() % 64) >> 4;
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

#endif /* _ETSOC_ISA_HART_H_ */