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
*       device-ops-api commands coming from host
*
*   FUNCTIONS
*
*       Host_Command_Handler
*
***********************************************************************/
#include "services/host_iface.h"
#include "services/host_cmd_hdlr.h"
#include "services/log1.h"

/************************************************************************
*
*   FUNCTION
*
*       Host_Command_Handler
*  
*   DESCRIPTION
*
*       Handle a host command and process its response.
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
int8_t Host_Command_Handler(void* command_buffer)
{
    int8_t status = STATUS_SUCCESS;
    struct cmd_header_t *hdr = command_buffer;

    switch (hdr->cmd_hdr.msg_id) 
    {
        case DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD:
        {
            struct device_ops_echo_cmd_t *cmd = (void *)hdr;
            struct device_ops_echo_rsp_t rsp;

            Log_Write(LOG_LEVEL_INFO, 
                "DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD\r\n");

            /* Construct and transmit response */
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id = 
                DEV_OPS_API_MID_DEVICE_OPS_ECHO_RSP;
            rsp.echo_payload = cmd->echo_payload;
            int8_t cq_status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

            if (cq_status != STATUS_SUCCESS) 
            {
                Log_Write(LOG_LEVEL_ERROR, "%s %d %s",
                    "HostCmdHandler:Host_Iface_CQ_Push_Cmd failed (error):", 
                    cq_status, "\r\n");
            } 
            else 
            {
                Log_Write(LOG_LEVEL_DEBUG, "%s%X%s",
                "HostCmdHandler:Host_Iface_CQ_Push_Cmd: ECHO_RSP Echo_Payload = 0x", 
                rsp.echo_payload, "\r\n");
            }

            break;
        }
        default: 
        {
            status = -1;
        }
    }

    return status;
}
