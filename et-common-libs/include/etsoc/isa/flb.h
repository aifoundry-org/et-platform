/***********************************************************************/
/*! \copyright
  Copyright (C) 2023 Esperanto Technologies Inc.
  The copyright to the computer program(s) herein is the
  property of Esperanto Technologies, Inc. All Rights Reserved.
  The program(s) may be used and/or copied only with
  the written permission of Esperanto Technologies and
  in accordance with the terms and conditions stipulated in the
  agreement/contract under which the program(s) have been supplied.
*/
/***********************************************************************/
/*! \file flb.h
    \brief Header/Interface description for FLB services
*/
/***********************************************************************/
#ifndef FLB_H
#define FLB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "etsoc/isa/esr_defines.h"
#include <stdint.h>

/*! \fn INIT_FLB(shire, barrier)
    \brief Initialize FL barrier
    \param shire shire to init FLBarrier
    \param barrier barrier value
    \return FL barrier value
    \syncops Implementation of INIT_FLB api
*/
#define INIT_FLB(shire, barrier) \
    (*((volatile uint64_t *)ESR_SHIRE(shire, FAST_LOCAL_BARRIER0) + barrier) = 0U)

/*! \fn READ_FLB(shire, barrier)
    \brief read FL barrier value
    \param shire shire to read FLBarrier
    \param barrier barrier value
    \return FL barrier value
    \syncops Implementation of READ_FLB api
*/
#define READ_FLB(shire, barrier) \
    (*((volatile uint64_t *)ESR_SHIRE(shire, FAST_LOCAL_BARRIER0) + barrier))

/*! \fn WAIT_FLB(threads, barrier, result)
    \brief The FLBarrier instruction allows one ET-Minion to "join" one of the eight FLB barriers. Thread parameter holds the
    expected number of threads (minus 1) that will join in the barrier as well as the barrier number. The instruction atomically
    increments the indicated barrier counter. After incrementing, the barrier counter is compared to the val[12:5] value plus 1. On
    match, result is set to 1 and the barrier counter is reset to zero. Otherwise, result is set to 0. 
    \param threads number of threads
    \param barrier barrier counter value
    \param result result of wait, if thread is last to join 0x1 , otherwise it will return a 0x0.
    \return FL barrier value
    \syncops Implementation of WAIT_FLB api
*/
#define WAIT_FLB(threads, barrier, result)                           \
    do                                                               \
    {                                                                \
        const uint64_t val = ((threads - 1U) << 5U) + barrier;       \
        asm volatile("csrrw %0, flb, %1" : "=r"(result) : "r"(val)); \
    } while (0)

/*! \def FLB_COUNT
    \brief Define for FLB count.
*/
#define FLB_COUNT 32

#ifdef __cplusplus
}
#endif

#endif // FLB_H
