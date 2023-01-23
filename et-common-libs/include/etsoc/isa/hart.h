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
    \brief Get current hart ID from where this call is made
    \return Hart ID
    \hartsvclib Implementation of get hart id api
    \example get_hart_id.c
    Example(s) of using get_hart_id api
*/
static inline unsigned int __attribute__((always_inline, const)) get_hart_id(void)
{
    uint64_t ret;

    __asm__ __volatile__("csrr %0, hartid" : "=r"(ret));

    return (unsigned int)ret;
}

/*! \fn static inline unsigned int get_shire_id(void)
    \brief Get current shire ID from where this call is made
    \return Shire ID
    \hartsvclib Implementation of shire id api
    \example get_shire_id.c
    Example(s) of using get_shire_id api
*/
static inline unsigned int __attribute__((always_inline, const)) get_shire_id(void)
{
    return get_hart_id() >> 6;
}

/*! \fn static inline unsigned int get_neighborhood_id(void)
    \brief Get current neighborhood ID from where this call is made 
    \return Neighborhood ID
    \hartsvclib Implementation of get neighborhood api
    \example get_neighborhood_id.c
    Example(s) of using get_neighborhood_id api
*/
static inline unsigned int __attribute__((always_inline, const)) get_neighborhood_id(void)
{
    return (get_hart_id() % 64) >> 4;
}

/*! \fn static inline unsigned int get_minion_id(void)
    \brief Get current minion ID from where this call is made 
    \return Minion ID
    \hartsvclib Implementation of minion id api
    \example get_minion_id.c
    Example(s) of using get_minion_id api
*/
static inline unsigned int __attribute__((always_inline, const)) get_minion_id(void)
{
    return get_hart_id() >> 1;
}

/*! \fn static inline unsigned int get_thread_id(void)
    \brief Get current thread ID from where this call is made 
    \return Thread ID
    \hartsvclib Implementation thread id api
    \example get_thread_id.c
    Example(s) of using get_thread_id api
*/
static inline unsigned int __attribute__((always_inline, const)) get_thread_id(void)
{
    return get_hart_id() & 1;
}

#ifdef __cplusplus
}
#endif

#endif /* _ETSOC_ISA_HART_H_ */