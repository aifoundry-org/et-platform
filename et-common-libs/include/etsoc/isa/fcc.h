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
/*! \file fcc.h
    \brief Header/Interface description for FCC services
*/
/***********************************************************************/
#ifndef FCC_H
#define FCC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "etsoc/isa/esr_defines.h"
#include <stdint.h>

/*! \def THREAD_0
    \brief A define for thread  0.
*/
#define THREAD_0 0

/*! \def THREAD_0
    \brief A define for thread  1.
*/
#define THREAD_1 1

/*! \enum fcc_t
    \brief enum defining FCC IDs.
*/
typedef enum { FCC_0 = 0, FCC_1 = 1 } fcc_t;

/*! \def SEND_FCC(shire, thread, fcc, bitmask)
    \brief Send fast credit counter to a thread
    \param shire shire to send the credit to, 0-32 or 0xFF for "this shire"
    \param thread thread to send credit to, 0 or 1
    \param fcc fast credit counter to send credit to, 0 or 1
    \param bitmask which minion to send credit to. bit 0 = minion 0, bit 31 = minion 31
    \return none
    \syncops Implementation of SEND_FCC macro
*/
#define SEND_FCC(shire, thread, fcc, bitmask) \
    (*((volatile uint64_t *)ESR_SHIRE(shire, FCC_CREDINC_0) + (thread * 2) + fcc) = bitmask)

/*! \def WAIT_FCC(fcc)
    \brief fast credit counter to block on, 0 or 1, it will attempt to decrement the value in
    the credit counter COUNTER. If the selected credit counter value is zero, execution of the 
    issuing hart will stall until a credit is received by another hart, at which point execution will
    resume and the counter value will be decremented by one
    \param fcc fast credit counter number, 0 or 1
    \return none
    \syncops Implementation of WAIT_FCC macro
*/
#define WAIT_FCC(fcc) asm volatile("csrwi fcc, %0" : : "I"(fcc))

/*! \fn static inline void wait_fcc(fcc_t fcc)
    \brief fast credit counter to block on, 0 or 1, it will attempt to decrement the value in
    the credit counter COUNTER. If the selected credit counter value is zero, execution of the 
    issuing hart will stall until a credit is received by another hart, at which point execution will
    resume and the counter value will be decremented by one
    \param fcc fast credit counter number, 0 or 1
    \return none
    \syncops Implementation of wait_fcc api
*/
static inline void wait_fcc(fcc_t fcc)
{
    asm volatile("csrw fcc, %0" : : "r"(fcc));
}

/*! \fn read_fcc(fcc_t fcc)
    \brief read fast credit counter value, it will decrement FCC if the counter > 0, 
    \param fcc fast credit counter to send credit to, 0 or 1
    \return fcc value
    \syncops Implementation of read_fcc api
*/
static inline uint64_t read_fcc(fcc_t fcc)
{
    uint64_t temp;
    uint64_t val;

    asm volatile("   csrr  %0, fccnb  \n" // read FCCNB
                 "   beqz  %2, 1f     \n" // if FCC1, shift FCCNB 31:16 down to 15:0
                 "   srli  %0, %0, 16 \n"
                 "1: lui   %1, 0x10   \n" // mask with 0xFFFF
                 "   addiw %1, %1, -1 \n"
                 "   and   %0, %0, %1 \n"
                 : "=&r"(val), "=r"(temp)
                 : "r"(fcc));

    return val;
}

/*! \fn static inline void init_fcc(fcc_t fcc)
    \brief Funtion to initialize FCC.
    \param fcc fast credit counter to init
    \return fcc value
    \syncops Implementation of init_fcc api
*/
static inline void init_fcc(fcc_t fcc)
{
    // Consume all credits
    for (uint64_t i = read_fcc(fcc); i > 0; i--)
    {
        wait_fcc(fcc);
    }
}

#ifdef __cplusplus
}
#endif

#endif // FCC_H
