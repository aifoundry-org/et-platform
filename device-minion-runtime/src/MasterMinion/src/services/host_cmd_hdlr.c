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
/*! \file host_cmd_hdlr.c
    \brief A C module that implements the host command handler
    responsible for handling all commands from host

    Public interfaces:
        Host_Command_Handler
*/
/***********************************************************************/
#include "services/host_cmd_hdlr.h"
#include "services/host_iface.h"
#include "services/worker_iface.h"
#include "workers/cqw.h"
#include "services/log1.h"
#include "pmu.h"

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
*       start_cycle      Cycle count to measure wait latency
*
*   OUTPUTS
*
*       int8_t           Successful status or error code.
*
***********************************************************************/
int8_t Host_Command_Handler(void* command_buffer, uint64_t start_cycle)
{
    int8_t status = STATUS_SUCCESS;
    struct cmd_header_t *hdr = command_buffer;

    switch (hdr->cmd_hdr.msg_id) 
    {
        case DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD:
        {
            struct device_ops_api_compatibility_rsp_t rsp;
            
            Log_Write(LOG_LEVEL_DEBUG, "%s", 
                "HostCommandHandler: Processing COMPATIBILITY_CMD \r\n");

            /* Construct and transmit response */
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id = 
                DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP;
            rsp.response_info.rsp_hdr.size = 
                sizeof(struct device_ops_api_compatibility_rsp_t);
            rsp.major = DEVICE_OPS_API_MAJOR;
            rsp.minor = DEVICE_OPS_API_MINOR;
            rsp.patch = DEVICE_OPS_API_PATCH;

            status = Worker_Iface_Push_Cmd(TO_CQW_FIFO, &rsp, sizeof(rsp));
            if(status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG, "%s", 
                    "HostCommandHandler: Pushed COMPATIBILITY_CMD \
                    response to CQW \r\n");
                CQW_Notify();
            }
            else
            {
                Log_Write(LOG_LEVEL_DEBUG, "%s", 
                    "HostCommandHandler: WorkerIface Push Failed \r\n");
            }
            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD:
        {
            struct device_ops_device_fw_version_cmd_t *cmd = (void *)hdr;
            struct device_ops_fw_version_rsp_t rsp;

            Log_Write(LOG_LEVEL_DEBUG, "%s", 
                "HostCommandHandler: Processing FW_VERSION_CMD \r\n");

            /* Construct and transmit response */
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id = 
                DEV_OPS_API_MID_DEVICE_OPS_FW_VERSION_RSP;
            rsp.response_info.rsp_hdr.size = 
                sizeof(struct device_ops_fw_version_rsp_t);
            if (cmd->firmware_type == DEV_OPS_FW_TYPE_MASTER_MINION_FW) 
            {
                /* TODO: implement proper logic to fetch and 
                return firmware version */
                rsp.major = 1;
                rsp.minor = 0;
                rsp.patch = 0;
                rsp.type = DEV_OPS_FW_TYPE_MASTER_MINION_FW;
            } 
            else if (cmd->firmware_type == DEV_OPS_FW_TYPE_MACHINE_MINION_FW) 
            {
                /*TODO: implement proper logic to fetch and return 
                firmware version */
                rsp.major = 1;
                rsp.minor = 0;
                rsp.patch = 0;
                rsp.type = DEV_OPS_FW_TYPE_MACHINE_MINION_FW;
            } 
            else if (cmd->firmware_type == DEV_OPS_FW_TYPE_WORKER_MINION_FW) 
            {
                /*TODO: implement proper logic to fetch and return 
                firmware version */
                rsp.major = 1;
                rsp.minor = 0;
                rsp.patch = 0;
                rsp.type = DEV_OPS_FW_TYPE_WORKER_MINION_FW;
            }
            
            status = Worker_Iface_Push_Cmd(TO_CQW_FIFO, &rsp, sizeof(rsp));
            if(status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG, "%s", 
                    "HostCommandHandler: Pushed FW_VERSION_CMD \
                    response to CQW \r\n");
                CQW_Notify();
            }
            else
            {
                Log_Write(LOG_LEVEL_DEBUG, "%s", 
                    "HostCommandHandler: WorkerIface Push Failed \r\n");
            }

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD:
        {
            struct device_ops_echo_cmd_t *cmd = (void *)hdr;
            struct device_ops_echo_rsp_t rsp;

            Log_Write(LOG_LEVEL_DEBUG, "%s", 
                "HostCommandHandler: Processing ECHO_CMD \r\n");

            /* Construct and transmit response */
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id = 
                DEV_OPS_API_MID_DEVICE_OPS_ECHO_RSP;
            rsp.response_info.rsp_hdr.size = 
                sizeof(struct device_ops_echo_rsp_t);
            rsp.echo_payload = cmd->echo_payload;            

            status = Worker_Iface_Push_Cmd(TO_CQW_FIFO, &rsp, sizeof(rsp));
            if(status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG, "%s", 
                    "HostCommandHandler: Pushed ECHO_CMD response to CQW \r\n");
                CQW_Notify();
            }
            else
            {
                Log_Write(LOG_LEVEL_DEBUG, "%s", 
                    "HostCommandHandler: WorkerIface Push Failed \r\n");
            }

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD:
        case DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_CMD:
        case DEV_OPS_API_MID_DEVICE_OPS_KERNEL_STATE_CMD:
        {
            (void)calculate_latency(start_cycle);
            Log_Write(LOG_LEVEL_DEBUG, "%s", 
                "HostCommandHandler: received unsupported cmd \r\n");

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD:
        case DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD:
        {
            (void)calculate_latency(start_cycle);
            Log_Write(LOG_LEVEL_DEBUG, "%s", 
                "HostCommandHandler: received unsupported cmd \r\n");

            break;
        }
        default: 
        {
            Log_Write(LOG_LEVEL_DEBUG, "%s", 
                "HostCommandHandler: received unsupported cmd \r\n");

            status = -1;
            break;
        }
    }

    return status;
}
