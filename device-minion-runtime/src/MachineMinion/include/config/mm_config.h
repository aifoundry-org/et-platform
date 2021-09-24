/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
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

/*! \def MM_HART_MASK
    \brief Master Minion Hart mask
*/
#define MM_HART_MASK             (0xFFFFFFFFUL)

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

/*! \def MM_DVFS_NORMALIZATION_OFF
    \brief Master Minion DVFS with PLL normalization off.
*          According to Movellus once we normalize PLL clock at higher frequency,
*          we can switch to lower frequency with normalization off.
*          Lock is faster achieved then.
*/
#define MM_DVFS_NORMALIZATION_OFF   0

/*! \def MM_DVFS_USE_FCW
    \brief Use FCW sequence for DVFS.
*          Reprogramming PLL by changing just a few FCW regs with pre-calculated values.
*/
#define MM_DVFS_USE_FCW           0

/*! \def MM_DVFS_POLL_FOR_LOCK
    \brief Poll for PLL lock.
*          Poll lock bit or just wait some predefined time.
*/
#define MM_DVFS_POLL_FOR_LOCK     0

/*! \def INITIAL_MINION_FREQ
    \brief Initial Minion PLL frequency
*/
#define INITIAL_MINION_FREQ       650

#endif /* __MM_CONFIG_H__ */
