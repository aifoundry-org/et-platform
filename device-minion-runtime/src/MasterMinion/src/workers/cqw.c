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
*       This file consists the Completion Queue Worker implementation.
*
*   FUNCTIONS
*
*       CQW_Launch
*
***********************************************************************/
#include    "workers/cqw.h"
#include    "services/log1.h"
#include    "vq.h"

typedef struct cqw_cb_ {
    global_fcc_flag_t   cqw_fcc_flag;
    vq_cb_t             *vq_cb;
} cqw_cb_t;

/*! \var cqw_cb_t CQW_CB
    \brief Global Completion Queue Worker Control Block
    \warning Not thread safe!
*/
static cqw_cb_t CQW_CB __attribute__((aligned(8)))={0};

/************************************************************************
*
*   FUNCTION
*
*       CQW_Init
*  
*   DESCRIPTION
*
*       Initialize completion queue worker
*
*   INPUTS
*
*       uNone
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void CQW_Init(void)
{
    /* Initialize FCC sync flags */
    global_fcc_flag_init(&CQW_CB.cqw_fcc_flag);
    
    return;
}

/************************************************************************
*
*   FUNCTION
*
*       CQW_Launch
*  
*   DESCRIPTION
*
*       Launch a Completion Queue Worker on HART ID requested
*
*   INPUTS
*
*       hart_id   HART ID to launch the dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void CQW_Launch(uint32_t hart_id)
{
    Log_Write(LOG_LEVEL_DEBUG, "%s = %d %s", "SQW = ", hart_id, 
        "launched\r\n");

    /* Empty all FCCs */
    init_fcc(FCC_0);
    init_fcc(FCC_1);

    while(1)
    {
        /* Wait for CQ Worker notification from 
        Dispatcher, KW */
        global_fcc_flag_wait(&CQW_CB.cqw_fcc_flag);

    };
    
    return;
}