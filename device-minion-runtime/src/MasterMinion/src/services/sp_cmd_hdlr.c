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
/*! \file sp_cmd_hdlr.c
    \brief A C module that implements the Service Processor Command
    handler

    Public interfaces:
        SP_Command_Handler
*/
/***********************************************************************/
#include "services/sp_iface.h"
#include "services/sp_cmd_hdlr.h"
#include "services/log.h"
#include "mm_sp_cmd_spec.h"

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
*
*   OUTPUTS
*
*       int8_t           Successful status or error code.
*
***********************************************************************/
int8_t SP_Command_Handler(void* command_buffer)
{
    int8_t status = STATUS_SUCCESS;
    struct mm_sp_cmd_hdr_t *hdr = command_buffer;

    switch (hdr->msg_id)
    {
        case SP2MM_CMD_ECHO:
        {
            struct sp2mm_echo_rsp_t echo_rsp;
            struct sp2mm_echo_cmd_t *cmd = (void*)hdr;

            echo_rsp.msg_hdr.msg_id = SP2MM_RSP_ECHO;
            echo_rsp.payload = cmd->payload;

            SP_Iface_CQ_Push_Cmd(&echo_rsp, sizeof(echo_rsp));

            Log_Write(LOG_LEVEL_DEBUG,
                "SPCommandHandler: received Echo cmd\r\n");
            break;
        }
        case SP2MM_CMD_UPDATE_FREQ:
        {
            struct sp2mm_update_freq_rsp_t update_freq_rsp;

            /* Do the work here */

            update_freq_rsp.msg_hdr.msg_id = SP2MM_RSP_UPDATE_FREQ;
            update_freq_rsp.status = STATUS_SUCCESS; /* fake success */

            SP_Iface_CQ_Push_Cmd(&update_freq_rsp, sizeof(update_freq_rsp));

            Log_Write(LOG_LEVEL_DEBUG,
                "SPCommandHandler: received Update Frequency cmd\r\n");
            break;
        }
        case SP2MM_CMD_TEARDOWN_MM:
        {
            struct sp2mm_teardown_mm_rsp_t teardown_rsp;

            /* Do the work here */

            teardown_rsp.msg_hdr.msg_id = SP2MM_RSP_TEARDOWN_MM;
            teardown_rsp.status = STATUS_SUCCESS; /* fake success */

            SP_Iface_CQ_Push_Cmd(&teardown_rsp, sizeof(teardown_rsp));

            Log_Write(LOG_LEVEL_DEBUG,
                "SPCommandHandler: received Teardown MM cmd\r\n");
            break;
        }
        case SP2MM_CMD_QUIESCE_TRAFFIC:
        {
            struct sp2mm_quiesce_traffic_rsp_t quiesce_rsp;

            /* Do the work here */

            quiesce_rsp.msg_hdr.msg_id = SP2MM_RSP_QUIESCE_TRAFFIC;
            quiesce_rsp.status = STATUS_SUCCESS; /* fake success */

            SP_Iface_CQ_Push_Cmd(&quiesce_rsp, sizeof(quiesce_rsp));

            Log_Write(LOG_LEVEL_DEBUG,
                "SPCommandHandler: received Quiesce cmd\r\n");
            break;
        }
        case SP2MM_CMD_GET_TRACE_BUFF_CONTROL_STRUCT:
        {
            Log_Write(LOG_LEVEL_DEBUG,
                "SPCommandHandler: received Get Trace Buff Struct cmd\r\n");
            break;
        }
        default:
        {
            Log_Write(LOG_LEVEL_ERROR,
                "SPCommandHandler:ERROR:received unsupported cmd\r\n");
            status = -1;

            break;
        }
    }

    return status;
}
