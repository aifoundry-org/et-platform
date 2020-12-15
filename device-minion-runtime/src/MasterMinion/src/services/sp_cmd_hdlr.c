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
*       This file consists implementation for handling of 
*       commands coming from SP
*
*   FUNCTIONS
*
*       SP_Command_Handler
*
***********************************************************************/
#include "services/sp_iface.h"
#include "services/sp_cmd_hdlr.h"
#include "services/log1.h"

/************************************************************************
*
*   FUNCTION
*
*       SP_Command_Handler
*  
*   DESCRIPTION
*
*       Handle a SP command and process its response.
*
*   INPUTS
*
*       command_buffer   Buffer containing command to process
*       command_size     Size of the command
*
*   OUTPUTS
*
*       int8_t           Successful status or error code.
*
***********************************************************************/
int8_t SP_Command_Handler(void* command_buffer, uint16_t command_size)
{
    (void) command_buffer;
    (void) command_size;
    
    /* TODO: Implement command parsing and processing logic here */

    /* TODO: Use sp_iface.h provided SP_Iface_CQ_Push_Cmd API to 
    respond to SP */

    return 0;
}
