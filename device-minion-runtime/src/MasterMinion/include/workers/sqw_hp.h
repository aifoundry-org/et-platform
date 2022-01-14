/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/***********************************************************************/
/*! \file sqw_hp.h
    \brief A C header that defines the High Priority Submission Queue
    Worker's public interfaces.
*/
/***********************************************************************/
#ifndef SQW_HP_DEFS_H
#define SQW_HP_DEFS_H

/* mm_rt_svcs */
#include <etsoc/common/common_defs.h>
#include <etsoc/isa/sync.h>
#include <etsoc/drivers/pmu/pmu.h>
#include <transports/vq/vq.h>

/* mm specific headers */
#include "config/mm_config.h"

/*! \def SQW_MAX_HART_ID
    \brief A macro that provides the maximum HART ID the SQW HP is configued
    to execute on.
*/
#define SQW_HP_MAX_HART_ID (SQW_HP_BASE_HART_ID + (SQW_HP_NUM * HARTS_PER_MINION))

/*! \def SQW_WORKER_0
    \brief A macro that provdies the minion index of the first High Priority
    Submission Queue worker within the master shire.
*/
#define SQW_HP_WORKER_0 ((SQW_HP_BASE_HART_ID - MM_BASE_ID) / HARTS_PER_MINION)

/*! \fn void SQW_HP_Init(void)
    \brief Initialize resources used by the HP Submission Queue Worker
    \return None
*/
void SQW_HP_Init(void);

/*! \fn void SQW_HP_Notify(uint8_t sqw_hp_idx)
    \brief High Priority Submission Queue Worker notify for given HP Submission Queue index
    \param sqw_hp_idx HP Submission Queue Worker index
    \return none
*/
void SQW_HP_Notify(uint8_t sqw_hp_idx);

/*! \fn void SQW_HP_Launch(uint32_t sqw_hp_idx)
    \brief Launch the High priority Submission Queue Worker
    \param sqw_hp_idx Queue Worker index
    \return none
*/
__attribute__((noreturn)) void SQW_HP_Launch(uint32_t sqw_hp_idx);

/*! \fn SQW_HP_Decrement_Command_Count(uint8_t sqw_hp_idx)
    \brief Decrement command count for the given HP Submission Queue Worker
    \param sqw_hp_idx Submission Queue Worker index
    \return none
*/
void SQW_HP_Decrement_Command_Count(uint8_t sqw_hp_idx);

/*! \fn SQW_HP_Increment_Command_Count(uint8_t sqw_hp_idx)
    \brief Increment command count for the given HP Submission Queue Worker
    \param sqw_hp_idx Submission Queue Worker index
    \return none
*/
void SQW_HP_Increment_Command_Count(uint8_t sqw_hp_idx);

#endif /* SQW_HP_DEFS_H */
