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
*       This file DMA Worker.
*
*   FUNCTIONS
*
*       DMAW_Launch
*
***********************************************************************/
#include    "workers/dmaw.h"
#include    "services/log1.h"

/************************************************************************
*
*   FUNCTION
*
*       DMAW_Launch
*  
*   DESCRIPTION
*
*       Launch a DMA Worker on HART ID requested
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
void DMAW_Launch(uint32_t hart_id)
{
    (void) hart_id;

    //Log_Write(LOG_LEVEL_DEBUG, "%s = %d %s", "DMAW ="
    //            , hart_id, "launched\r\n");
    
    return;
}