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
/*! \file cqw.h
    \brief A C header that defines the Completion Queue Worker's public 
    interfaces.
*/
/***********************************************************************/

#ifndef CQW_DEFS_H
#define CQW_DEFS_H

#include "config/mm_config.h"
#include "common_defs.h"
#include "sync.h"

/*! \def CQW_MAX_HART_ID
    \brief A macro that provides the maximum HART ID the Completion Queue 
    worker is configued to execute on.
*/
#define  CQW_MAX_HART_ID      CQW_BASE_HART_ID + CQW_NUM

/*! \def DMAW_MAX_HART_ID
    \brief A macro that provdies the minion index of the first Completion
    Queue worker within the master shire.
*/
#define  CQW_WORKER_0         ((CQW_BASE_HART_ID - MM_BASE_ID)/2)

/*! \fn void CQW_Init(void)
    \brief Initialize resources used by the Completion Queue Worker
    \return none
*/
void CQW_Init(void);

/*! \fn void CQW_Notify(uint8_t cqw_idx)
    \brief Notify Completion Queue Worker
    \return none
*/
void CQW_Notify(void);

/*! \fn void CQW_Launch(uint32_t hart_id, uint32_t cqw_idx)
    \brief Launch the Completion Queue Worker
    \param hart_id HART ID on which the Completion Queue Worker should 
    be launched
    \return none
*/
void CQW_Launch(uint32_t hart_id);

#endif /* CQW_DEFS_H */