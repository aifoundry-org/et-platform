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
/***********************************************************************/
/*! \file sqw.h
    \brief A C header that defines the Submission Queue Worker's
    public interfaces.
*/
/***********************************************************************/
#ifndef SQW_DEFS_H
#define SQW_DEFS_H

#include "config/mm_config.h"
#include "common_defs.h"
#include "sync.h"
#include "pmu.h"
#include "vq.h"

/*! \def SQW_MAX_HART_ID
    \brief A macro that provides the maximum HART ID the SQW is configued
    to execute on.
*/
#define     SQW_MAX_HART_ID      (SQW_BASE_HART_ID + (SQW_NUM * WORKER_HART_FACTOR))

/*! \def SQW_WORKER_0
    \brief A macro that provdies the minion index of the first Submission
    Queue worker within the master shire.
*/
#define     SQW_WORKER_0         ((SQW_BASE_HART_ID - MM_BASE_ID) / HARTS_PER_MINION)

/*! \fn void SQW_Init(void)
    \brief Initialize resources used by the Submission Queue Worker
    \return None
*/
void SQW_Init(void);

/*! \fn void SQW_Notify(uint8_t sqw_idx)
    \brief Submission Queue Worker notify for given Submission Queue index
    \param sqw_idx Submission Queue Worker index
    \return none
*/
void SQW_Notify(uint8_t sqw_idx);

/*! \fn void SQW_Launch(uint32_t hart_id, uint32_t sqw_idx)
    \brief Launch the Submission Queue Worker
    \param hart_id HART ID on which the Submission Queue Worker should be launched
    \param sqw_idx Queue Worker index
    \return none
*/
void SQW_Launch(uint32_t hart_id, uint32_t sqw_idx);

/*! \fn SQW_Decrement_Command_Count(uint8_t sqw_idx)
    \brief Decrement command count for the given Submission Queue Worker
    \param sqw_idx Submission Queue Worker index
    \return none
*/
void SQW_Decrement_Command_Count(uint8_t sqw_idx);

/*! \fn SQW_Increment_Command_Count(uint8_t sqw_idx)
    \brief Increment command count for the given Submission Queue Worker
    \param sqw_idx Submission Queue Worker index
    \return none
*/
void SQW_Increment_Command_Count(uint8_t sqw_idx);

/*! \fn void SQW_Command_Barrier_Timeout_Cb(uint8_t sqw_idx)
    \brief Callback for SQW command barrier timeout
    \param sqw_idx Submission Queue Worker index
    \return none
*/
void SQW_Command_Barrier_Timeout_Cb(uint8_t sqw_idx);

#endif /* SQW_DEFS_H */
