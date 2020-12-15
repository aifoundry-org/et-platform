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
*       This file implements the Submission Queue Worker.
*
*   FUNCTIONS
*
*       SQW_Worker
*
***********************************************************************/
#include "config/mm_config.h"
#include "workers/sqw.h"
#include "services/log1.h"
#include "sync.h"

/*! \var global_fcc_flag_t SQ_Worker_Sync
    \brief Global array of flags for SQ Worker synchronization
    \warning Not thread safe!
*/
static global_fcc_flag_t SQ_Worker_Sync[MM_SQ_COUNT] = { 0 };

/************************************************************************
*
*   FUNCTION
*
*       SQW_Init
*  
*   DESCRIPTION
*
*       Initialize resources needed by SQ Workers
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void SQW_Init(void)
{
    /* Initialize the SQ Worker sync flags */ 
    for (uint8_t i = 0; i < MM_SQ_COUNT; i++) 
    {
        global_fcc_flag_init(&SQ_Worker_Sync[i]);
    }
    
    return;
}

/************************************************************************
*
*   FUNCTION
*
*       SQW_Launch
*  
*   DESCRIPTION
*
*       Launch a Submission Queue Worker on HART ID requested
*
*   INPUTS
*
*       uint32_t   HART ID to launch the dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void SQW_Launch(uint32_t hart_id)
{
    (void) hart_id;

    //Log_Write(LOG_LEVEL_DEBUG, "%s = %d %s", "SQW = "
    //            , hart_id, "launched\r\n");

    /* Empty all FCCs */
    init_fcc(FCC_0);
    init_fcc(FCC_1);
    
    return;
}