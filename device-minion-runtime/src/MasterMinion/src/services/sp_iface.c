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
/*! \file sp_iface.c
    \brief A C module that implements the Service Processor Interface
    services

    Public interfaces:
        SP_Iface_Processing
*/
/***********************************************************************/
#include "services/sp_iface.h"
#include "services/log.h"
#include "sp_mm_comms_spec.h"

static int8_t sp_command_handler(void* cmd_buffer);
static int8_t sp_response_handler(void* rsp_buffer);

/************************************************************************
*
*   FUNCTION
*
*       sp_response_handler
*
*   DESCRIPTION
*
*       A local scope helper function to process SP to MM response messages
*
***********************************************************************/
static int8_t sp_response_handler(void* rsp_buffer)
{
    int8_t status = STATUS_SUCCESS;
    struct dev_cmd_hdr_t *hdr = rsp_buffer;

    Log_Write(LOG_LEVEL_DEBUG, "SP_Response_Handler:hdr:%s%d%s%d%s",
            ":msg_id:",hdr->msg_id,
            ":msg_size:",hdr->msg_size, "\r\n");

    switch (hdr->msg_id)
    {
        case MM2SP_RSP_ECHO:
        {
            Log_Write(LOG_LEVEL_DEBUG,
                "sp_response_handler: Received echo response 1 from SP ****\r\n");
            break;
        }
        default:
        {
            break;
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       sp_command_handler
*
*   DESCRIPTION
*
*       A local scope helper function to process supported SP to MM
*       commands
*
***********************************************************************/
static int8_t sp_command_handler(void* cmd_buffer)
{
    int8_t status = STATUS_SUCCESS;
    struct dev_cmd_hdr_t *hdr = cmd_buffer;

    Log_Write(LOG_LEVEL_DEBUG, "SP_Command_Handler:hdr:%s%d%s%d%s",
            ":msg_id:",hdr->msg_id,
            ":msg_size:",hdr->msg_size, "\r\n");

    switch (hdr->msg_id)
    {
        case SP2MM_CMD_ECHO:
        {
            struct sp2mm_echo_cmd_t *echo_cmd = (void*) hdr;

            Log_Write(LOG_LEVEL_DEBUG,
                    "SP_Command_Handler:Echo:%s%d%s%d%s0x%x%s",
                    ":msg_id:",hdr->msg_id,
                    ":msg_size:",hdr->msg_size,
                    ":payload", echo_cmd->payload,"\r\n");

            /* Implement functionality here .. */
            break;
        }
        case SP2MM_CMD_UPDATE_FREQ:
        {
            struct sp2mm_update_freq_cmd_t *update_active_freq_cmd =
                (void*) hdr;

            Log_Write(LOG_LEVEL_DEBUG,
                "SP_Command_Handler:UpdateActiveFreq:%s%d%s%d%s0x%x%s",
                ":msg_id:",hdr->msg_id,
                ":msg_size:",hdr->msg_size,
                ":payload", update_active_freq_cmd->freq,"\r\n");

            /* Implement functionality here .. */
            break;
        }
        case SP2MM_CMD_TEARDOWN_MM:
        {
            /* struct sp2mm_teardown_mm_cmd_t *teardown_mm_cmd = (void*) hdr; */

            Log_Write(LOG_LEVEL_DEBUG,
                "SP_Command_Handler:TearDownMM:%s%d%s%d%s",
                ":msg_id:",hdr->msg_id,
                ":msg_size:",hdr->msg_size, "\r\n");

            /* Implement functionality here .. */
            break;
        }
        case SP2MM_CMD_QUIESCE_TRAFFIC:
        {
            /* struct sp2mm_quiesce_traffic_cmd_t *quiese_traffic_cmd = (void*) hdr; */

            Log_Write(LOG_LEVEL_DEBUG,
                "SP_Command_Handler:QuieseTraffic:%s%d%s%d%s",
                ":msg_id:",hdr->msg_id,
                ":msg_size:",hdr->msg_size, "\r\n");

            /* Implement functionality here .. */
            break;
        }
        default:
        {
            Log_Write(LOG_LEVEL_ERROR,
                "SP_Command_Handler:UnsupportedCommandID.\r\n");

            break;
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_Processing
*
*   DESCRIPTION
*
*       Process MM2SP CQ response messages, and SP2MM SQ command messages
*       from Service Processor (SP)
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int8_t      status success or failure of Interface initialization
*
***********************************************************************/
int8_t SP_Iface_Processing(void)
{
    static uint8_t cmd_buff[64] __attribute__((aligned(64))) = { 0 };
    static uint8_t rsp_buff[64] __attribute__((aligned(64))) = { 0 };
    uint64_t cmd_length=0;
    uint64_t rsp_length=0;
    int8_t status = STATUS_SUCCESS;

    Log_Write(LOG_LEVEL_DEBUG, "SP_Iface_Processing..\r\n");

    rsp_length = (uint64_t) SP_Iface_Pop_Cmd_From_MM2SP_CQ(&rsp_buff[0]);

    if(rsp_length != 0 )
    {
        Log_Write(LOG_LEVEL_DEBUG,
            "SP_Iface_Processing:cmd_length: 0x%" PRIx64 "\r\n", rsp_length);

       status = sp_response_handler(rsp_buff);
    }

    cmd_length = (uint64_t) SP_Iface_Pop_Cmd_From_SP2MM_SQ(&rsp_buff[0]);

    if(cmd_length != 0 )
    {
        Log_Write(LOG_LEVEL_DEBUG,
            "SP_Iface_Processing:cmd_length: 0x%" PRIx64 "\r\n", cmd_length);

       status = sp_command_handler(cmd_buff);
    }

    return status;
}