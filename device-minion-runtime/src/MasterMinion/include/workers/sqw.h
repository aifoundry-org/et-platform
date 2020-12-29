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
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       Header/Interface description for the Submission Queue Worker
*       component's public interface
*
***********************************************************************/
#ifndef SQW_DEFS_H
#define SQW_DEFS_H

#include "config/mm_config.h"
#include "common_defs.h"
#include "sync.h"
#include "vq.h"

#define     SQW_MAX_HART_ID      (SQW_BASE_HART_ID + SQW_NUM)
#define     SQW_WORKER_0         ((SQW_BASE_HART_ID - MM_BASE_ID)/2)
                
/*! \fn void SQW_Init(void)
    \brief Initialize resources used by the Submission Queue Worker
    \param None
*/
void SQW_Init(void);

/*! \fn void SQW_Notify(uint8_t sqw_idx)
    \brief Submission Queue Worker notify
    \param [in] Submission Queue Worker index
*/
void SQW_Notify(uint8_t sqw_idx);

/*! \fn void SQW_Launch(uint32_t hart_id, uint32_t sqw_idx)
    \brief Launch the Submission Queue Worker
    \param [in] HART ID on which the Kernel Worker should be launched
    \param [in] Submission Queue Worker index
*/
void SQW_Launch(uint32_t hart_id, uint32_t sqw_idx);

#endif /* SQW_DEFS_H */