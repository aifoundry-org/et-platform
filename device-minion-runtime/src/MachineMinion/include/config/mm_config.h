/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file mm_config.h
    \brief This is the only build configuration file for the MachineMinion.
*/
/***********************************************************************/
#ifndef __MM_CONFIG_H__
#define __MM_CONFIG_H__

/*******************************************************************/
/* Definitions for MM Shire and its Harts                          */
/*******************************************************************/

/*! \def MM_SHIRE_ID
    \brief Virtual Master Minion Shire NOC ID
*/
#define MM_SHIRE_ID 32

/*! \def MM_DISPATCHER_HART_ID
    \brief Master Minion dispatcher hart ID
*/
#define MM_DISPATCHER_HART_ID 2048

/*! \def CM_SHIRE_ID_MASK
    \brief Virtual Compute Shire NOC ID Mask
*/
#define CM_SHIRE_ID_MASK 0xFFFFFFFFU

/*! \def MM_RT_THREADS
    \brief Define for Threads which participate in the Device Runtime FW management.
           Currently only lower 16 Minions (32 Harts) of the whole Minion Shire which
           participate in the Device Runtime.
*/
#define MM_RT_THREADS 0x0000FFFFU

/*! \def MM_COMPUTE_THREADS
    \brief Define for Threads which participate in the Device Runtime FW management.
         Currently the upper 16 Minions (32 Harts) of the whole Minion Shire
         participates in Compute Minion Kernel execution.
*/
#define MM_COMPUTE_THREADS 0xFFFF0000U

/*! \def INITIAL_MINION_FREQ
    \brief Initial Minion PLL frequency programmed by SP
*/
#define INITIAL_MINION_FREQ 650

#endif /* __MM_CONFIG_H__ */
