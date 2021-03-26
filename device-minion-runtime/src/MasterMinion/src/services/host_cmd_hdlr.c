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
#include "services/log.h"
#include "workers/kw.h"
#include "workers/dmaw.h"
#include "workers/sqw.h"
#include "pmu.h"

/************************************************************************
*
*   FUNCTION
*
*       Host_Command_Handler
*
*   DESCRIPTION
*
*       Process host command, and transmit response as needed
*
*   INPUTS
*
*       command_buffer   Buffer containing command to process
*       sqw_idx          Submission queue index
*       start_cycle      Cycle count to measure wait latency
*
*   OUTPUTS
*
*       int8_t           Successful status or error code.
*
***********************************************************************/
int8_t Host_Command_Handler(void* command_buffer, uint8_t sqw_idx,
    uint64_t start_cycles)
{
    int8_t status = STATUS_SUCCESS;
    struct cmd_header_t *hdr = command_buffer;
    exec_cycles_t cycles;

    switch (hdr->cmd_hdr.msg_id)
    {
        case DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD:
        {
            struct device_ops_api_compatibility_rsp_t rsp;

            Log_Write(LOG_LEVEL_DEBUG,
                "HostCommandHandler:Processing:API_COMPATIBILITY_CMD\r\n");

            /* Construct and transmit response */
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id =
                DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP;
            rsp.response_info.rsp_hdr.size =
                sizeof(struct device_ops_api_compatibility_rsp_t);
            rsp.major = DEVICE_OPS_API_MAJOR;
            rsp.minor = DEVICE_OPS_API_MINOR;
            rsp.patch = DEVICE_OPS_API_PATCH;

            status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

            if(status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG,
                    "HostCommandHandler:Pushed:API_COMPATIBILITY_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                    rsp.response_info.rsp_hdr.tag_id);
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "HostCommandHandler:HostIface:Push:Failed\r\n");
            }

            /* Decrement commands count being processed by given SQW */
            SQW_Decrement_Command_Count(sqw_idx);

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD:
        {
            struct device_ops_device_fw_version_cmd_t *cmd = (void *)hdr;
            struct device_ops_fw_version_rsp_t rsp;

            Log_Write(LOG_LEVEL_DEBUG,
                "HostCommandHandler:Processing:FW_VERSION_CMD\r\n");

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

            status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

            if(status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG,
                    "HostCommandHandler:Pushed:FW_VERSION_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                    rsp.response_info.rsp_hdr.tag_id);
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "HostCommandHandler:HostIface:Push:Failed\r\n");
            }

            /* Decrement commands count being processed by given SQW */
            SQW_Decrement_Command_Count(sqw_idx);

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD:
        {
            struct device_ops_echo_cmd_t *cmd = (void *)hdr;
            struct device_ops_echo_rsp_t rsp;

            Log_Write(LOG_LEVEL_DEBUG,
                "HostCommandHandler:Processing:ECHO_CMD\r\n");

            /* Construct and transmit response */
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id =
                DEV_OPS_API_MID_DEVICE_OPS_ECHO_RSP;
            rsp.response_info.rsp_hdr.size =
                sizeof(struct device_ops_echo_rsp_t);
            rsp.echo_payload = cmd->echo_payload;

            status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

            if(status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG,
                    "HostCommandHandler:Pushed:ECHO_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                    rsp.response_info.rsp_hdr.tag_id);
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "HostCommandHandler:HostIface:Push:Failed\r\n");
            }

            /* Decrement commands count being processed by given SQW */
            SQW_Decrement_Command_Count(sqw_idx);

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD:
        {
            struct device_ops_kernel_launch_cmd_t *cmd = (void *)hdr;
            struct device_ops_kernel_launch_rsp_t rsp;
            uint8_t kw_idx;

            Log_Write(LOG_LEVEL_DEBUG,
                "HostCommandHandler:Processing:KERNEL_LAUNCH_CMD\r\n");

            /* Compute Wait Cycles (cycles the command was sitting in SQ prior to launch)
               Snapshot current cycle */
            cycles.wait_cycles = (PMC_GET_LATENCY(start_cycles) & 0xFFFFFFF);
            cycles.start_cycles = ((uint32_t)PMC_Get_Current_Cycles() & 0xFFFFFFFF);

            /* Blocking call to launch kernel */
            status = KW_Dispatch_Kernel_Launch_Cmd(cmd, sqw_idx, &kw_idx);

            if(status == STATUS_SUCCESS)
            {
                /* Notify kernel worker to aggregate
                kernel completion responses, and construct
                and transmit command response to host
                completion queue */
                KW_Notify(kw_idx, &cycles);
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "HostCmdHdlr:KernelLaunch:Failed:Status:%d:CmdParams:code_start_address:%ld \
                    pointer_to_args:%ld shire_mask:%ld\r\n", status, cmd->code_start_address,
                    cmd->pointer_to_args, cmd->shire_mask);

                /* Construct and transit command response */
                rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
                rsp.response_info.rsp_hdr.msg_id =
                    DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP;
                rsp.response_info.rsp_hdr.size =
                    sizeof(struct device_ops_kernel_launch_rsp_t);
                rsp.cmd_wait_time = cycles.wait_cycles;
                rsp.cmd_execution_time = 0U;

                /* Populate the error type response */
                if (status == KW_ERROR_KERNEL_SHIRES_NOT_READY)
                {
                    rsp.status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_SHIRES_NOT_READY;
                }
                else if (status == KW_ERROR_KERNEL_INVALID_ADDRESS)
                {
                    rsp.status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_INVALID_ADDRESS;
                }
                else
                {
                    rsp.status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_ERROR;
                }

                status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

                if(status == STATUS_SUCCESS)
                {
                    Log_Write(LOG_LEVEL_DEBUG,
                        "HostCommandHandler:Pushed:KERNEL_LAUNCH_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                        rsp.response_info.rsp_hdr.tag_id);
                }
                else
                {
                    Log_Write(LOG_LEVEL_ERROR,
                        "HostCommandHandler:HostIface:Push:Failed\r\n");
                }

                /* Decrement commands count being processed by given SQW */
                SQW_Decrement_Command_Count(sqw_idx);
            }

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_CMD:
        {
            struct device_ops_kernel_abort_cmd_t *cmd = (void *)hdr;
            struct device_ops_kernel_abort_rsp_t rsp;

            Log_Write(LOG_LEVEL_DEBUG,
                "HostCommandHandler:Processing:KERNEL_ABORT_CMD\r\n");

            /* Dispatch kernel abort command */
            status = KW_Dispatch_Kernel_Abort_Cmd(cmd, sqw_idx);

            if(status != STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "HostCmdHdlr:KernelAbort:Failed:Status:%d:CmdParams:kernel_launch_tag_id:%d\r\n",
                    status, cmd->kernel_launch_tag_id);

                /* Construct and transit command response */
                rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
                rsp.response_info.rsp_hdr.msg_id =
                    DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP;
                rsp.response_info.rsp_hdr.size =
                    sizeof(struct device_ops_kernel_abort_rsp_t);

                /* Populate the error type response */
                if ((status == KW_ERROR_KERNEL_SLOT_NOT_FOUND) ||
                    (status == KW_ERROR_KERNEL_SLOT_NOT_USED))
                {
                    rsp.status =
                        DEV_OPS_API_KERNEL_ABORT_RESPONSE_INVALID_TAG_ID;
                }
                else
                {
                    rsp.status = DEV_OPS_API_KERNEL_ABORT_RESPONSE_ERROR;
                }

                status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

                if(status == STATUS_SUCCESS)
                {
                    Log_Write(LOG_LEVEL_DEBUG,
                        "HostCommandHandler:Pushed:KERNEL_ABORT_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                        rsp.response_info.rsp_hdr.tag_id);
                }
                else
                {
                    Log_Write(LOG_LEVEL_ERROR, "HostCommandHandler:HostIface:Push:Failed\r\n");
                }

                /* Decrement commands count being processed by given SQW */
                SQW_Decrement_Command_Count(sqw_idx);
            }

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD:
        {
            struct device_ops_data_read_cmd_t *cmd = (void *)hdr;
            struct device_ops_data_read_rsp_t rsp;
            dma_chan_id_e chan;

            /* Design Notes: Note a DMA write command from host will
            trigger the implementation to configure a DMA read channel
            on device to move data from host to device, similarly a read
            command from host will trigger the implementation to configure
            a DMA write channel on device to move data from device to host */
            Log_Write(LOG_LEVEL_DEBUG,
                "HostCommandHandler:Processing:DATA_READ_CMD\r\n");

            /* Obtain the next available DMA write channel */
            status = DMAW_Write_Find_Idle_Chan_And_Reserve(&chan, sqw_idx);

            if(status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG, "DMA_READ:channel_used:%d\r\n", chan);
                Log_Write(LOG_LEVEL_DEBUG, "DMA_READ:src_device_phy_addr:%" PRIx64 "\r\n",
                    cmd->src_device_phy_addr);
                Log_Write(LOG_LEVEL_DEBUG, "DMA_READ:dst_host_phy_addr:%" PRIx64 "\r\n",
                    cmd->dst_host_phy_addr);
                Log_Write(LOG_LEVEL_DEBUG, "DMA_READ:size:%" PRIx32 "\r\n", cmd->size);

                /* Compute Wait Cycles (cycles the command was sitting in SQ prior to launch)
                   Snapshot current cycle */
                cycles.wait_cycles = (PMC_GET_LATENCY(start_cycles) & 0xFFFFFFF);
                cycles.start_cycles = ((uint32_t)PMC_Get_Current_Cycles() & 0xFFFFFFFF);

                /* Initiate DMA write transfer */
                status = DMAW_Write_Trigger_Transfer(chan, cmd->src_device_phy_addr,
                    cmd->dst_host_phy_addr, cmd->size,
                    sqw_idx, hdr->cmd_hdr.tag_id, &cycles);
            }

            if(status != STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "HostCmdHdlr:DataReadCmd:Failed:Status:%d:CmdParams:dst_host_virt_addr:%ld\
                    :dst_host_phy_addr:%ld:src_device_phy_addr:%ld:size:%d\r\n",
                    status, cmd->dst_host_virt_addr, cmd->dst_host_phy_addr,
                    cmd->src_device_phy_addr, cmd->size);

                /* Construct and transmit command response */
                rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
                rsp.response_info.rsp_hdr.msg_id =
                    DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP;
                rsp.response_info.rsp_hdr.size = sizeof(rsp);
                /* Compute Wait Cycles (cycles the command was sitting
                in SQ prior to launch) Snapshot current cycle */
                rsp.cmd_wait_time = (PMC_GET_LATENCY(start_cycles) & 0xFFFFFFF);
                rsp.cmd_execution_time = 0U;

                /* Populate the error type response */
                if (status == DMAW_ERROR_TIMEOUT_FIND_IDLE_CHANNEL)
                {
                    rsp.status = DEV_OPS_API_DMA_RESPONSE_TIMEOUT_IDLE_CHANNEL_UNAVAILABLE;
                }
                else if(status == DMA_ERROR_INVALID_ADDRESS)
                {
                    rsp.status = DEV_OPS_API_DMA_RESPONSE_INVALID_ADDRESS;
                }
                else
                {
                    rsp.status = DEV_OPS_API_DMA_RESPONSE_ERROR;
                }

                status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

                if(status == STATUS_SUCCESS)
                {
                    Log_Write(LOG_LEVEL_DEBUG,
                        "HostCommandHandler:Pushed:DATA_READ_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                        rsp.response_info.rsp_hdr.tag_id);
                }
                else
                {
                    Log_Write(LOG_LEVEL_ERROR,
                        "HostCommandHandler:HostIface:Push:Failed\r\n");
                }

                /* Decrement commands count being processed by given SQW */
                SQW_Decrement_Command_Count(sqw_idx);
            }

            break;
        }

        case DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD:
        {
            struct device_ops_data_write_cmd_t *cmd = (void *)hdr;
            struct device_ops_data_write_rsp_t rsp;
            dma_chan_id_e chan;

            /* Design Notes: Note a DMA write command from host will trigger
            the implementation to configure a DMA read channel on device to move
            data from host to device, similarly a read command from host will
            trigger the implementation to configure a DMA write channel on device
            to move data from device to host */
            Log_Write(LOG_LEVEL_DEBUG,
                "HostCommandHandler:Processing:DATA_WRITE_CMD\r\n");

            /* Obtain the next available DMA read channel */
            status = DMAW_Read_Find_Idle_Chan_And_Reserve(&chan, sqw_idx);

            if(status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG, "DMA_WRITE:channel_used:%d\r\n", chan);
                Log_Write(LOG_LEVEL_DEBUG, "DMA_WRITE:src_host_virt_addr:%" PRIx64 "\r\n",
                    cmd->src_host_virt_addr);
                Log_Write(LOG_LEVEL_DEBUG, "DMA_WRITE:src_host_phy_addr:%" PRIx64 "\r\n",
                    cmd->src_host_phy_addr);
                Log_Write(LOG_LEVEL_DEBUG, "DMA_WRITE:dst_device_phy_addr:%" PRIx64 "\r\n",
                    cmd->dst_device_phy_addr);
                Log_Write(LOG_LEVEL_DEBUG, "DMA_WRITE:size:%" PRIx32 "\r\n", cmd->size);

                /* Compute Wait Cycles (cycles the command was sitting in SQ prior to launch)
                   Snapshot current cycle */
                cycles.wait_cycles = (PMC_GET_LATENCY(start_cycles) & 0xFFFFFFF);
                cycles.start_cycles = ((uint32_t)PMC_Get_Current_Cycles() & 0xFFFFFFFF);

                /* Initiate DMA read transfer */
                status = DMAW_Read_Trigger_Transfer(chan, cmd->src_host_phy_addr,
                    cmd->dst_device_phy_addr, cmd->size,
                    sqw_idx, hdr->cmd_hdr.tag_id, &cycles);
            }

            if(status != STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "HostCmdHdlr:DataWriteCmd:Failed:Status:%d:CmdParams:src_host_virt_addr:%ld\
                    :src_host_phy_addr:%ld:dst_device_phy_addr:%ld:size:%d\r\n",
                    status, cmd->src_host_virt_addr, cmd->src_host_phy_addr,
                    cmd->dst_device_phy_addr, cmd->size);

                /* Construct and transit command response */
                rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
                rsp.response_info.rsp_hdr.msg_id =
                    DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP;
                rsp.response_info.rsp_hdr.size = sizeof(rsp);
                /* Compute Wait Cycles (cycles the command was sitting
                in SQ prior to launch) Snapshot current cycle */
                rsp.cmd_wait_time = (PMC_GET_LATENCY(start_cycles) & 0xFFFFFFF);
                rsp.cmd_execution_time = 0U;

                /* Populate the error type response */
                if (status == DMAW_ERROR_TIMEOUT_FIND_IDLE_CHANNEL)
                {
                    rsp.status = DEV_OPS_API_DMA_RESPONSE_TIMEOUT_IDLE_CHANNEL_UNAVAILABLE;
                }
                else if(status == DMA_ERROR_INVALID_ADDRESS)
                {
                    rsp.status = DEV_OPS_API_DMA_RESPONSE_INVALID_ADDRESS;
                }
                else
                {
                    rsp.status = DEV_OPS_API_DMA_RESPONSE_ERROR;
                }

                status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

                if(status == STATUS_SUCCESS)
                {
                    Log_Write(LOG_LEVEL_DEBUG,
                        "HostCommandHandler:Pushed:DATA_WRITE_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                        rsp.response_info.rsp_hdr.tag_id);
                }
                else
                {
                    Log_Write(LOG_LEVEL_ERROR,
                        "HostCommandHandler:HostIface:Push:Failed\r\n");
                }

                /* Decrement commands count being processed by given SQW */
                SQW_Decrement_Command_Count(sqw_idx);
            }

            break;
        }
        default:
        {
            /* Decrement commands count being processed by given SQW */
            SQW_Decrement_Command_Count(sqw_idx);

            Log_Write(LOG_LEVEL_ERROR,
                "HostCommandHandler:UnsupportedCmd\r\n");
            status = -1;
            break;
        }
    }

    return status;
}
