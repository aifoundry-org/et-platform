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
    responsible for handling all Device Ops API commands from host

    Public interfaces:
        Host_Command_Handler
*/
/***********************************************************************/
#include "services/host_cmd_hdlr.h"
#include "services/host_iface.h"
#include "services/log.h"
#include "services/sw_timer.h"
#include "services/trace.h"
#include "services/cm_iface.h"
#include "services/sp_iface.h"
#include "workers/kw.h"
#include "workers/cw.h"
#include "workers/dmaw.h"
#include "workers/sqw.h"
#include "config/mm_config.h"
#include "pmu.h"
#include "device-common/cacheops.h"
#include "layout.h"

/*! \def TRACE_NODE_INDEX
    \brief Default trace node index in DMA list command when flag is set to extract Trace buffers.
           As flags are per command, not per transfer in the command, that is why when Trace flags
           are set only one transfer will be done which is the first one.
*/
#define TRACE_NODE_INDEX    0

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
    int8_t sw_timer_idx = -1;
    struct cmd_header_t *hdr = command_buffer;
    exec_cycles_t cycles;

    switch (hdr->cmd_hdr.msg_id)
    {
        case DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD:
        {
            struct device_ops_api_compatibility_rsp_t rsp;

            Log_Write(LOG_LEVEL_DEBUG,
                "SQ[%d] HostCommandHandler:Processing:API_COMPATIBILITY_CMD\r\n",sqw_idx);

            /* Construct and transmit response */
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id =
                DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP;
            rsp.response_info.rsp_hdr.size =
                sizeof(struct device_ops_api_compatibility_rsp_t) - sizeof(struct cmn_header_t);
            rsp.major = DEVICE_OPS_API_MAJOR;
            rsp.minor = DEVICE_OPS_API_MINOR;
            rsp.patch = DEVICE_OPS_API_PATCH;

            status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

            if(status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG,
                    "SQ[%d] HostCommandHandler:Pushed:API_COMPATIBILITY_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                    sqw_idx, rsp.response_info.rsp_hdr.tag_id);
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR, "SQ[%d]:HostCommandHandler:Tag_ID=%u:HostIface:Push:Failed\r\n",
                                            sqw_idx, hdr->cmd_hdr.tag_id);
                SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
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
                "SQ[%d] HostCommandHandler:Processing:FW_VERSION_CMD\r\n", sqw_idx);

            /* Construct and transmit response */
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id =
                DEV_OPS_API_MID_DEVICE_OPS_FW_VERSION_RSP;
            rsp.response_info.rsp_hdr.size =
                sizeof(struct device_ops_fw_version_rsp_t) - sizeof(struct cmn_header_t);
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
                    "SQ [%d] HostCommandHandler:Pushed:FW_VERSION_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                    sqw_idx, rsp.response_info.rsp_hdr.tag_id);
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR, "SQ[%d]:HostCommandHandler:Tag_ID=%u:HostIface:Push:Failed\r\n",
                                            sqw_idx, hdr->cmd_hdr.tag_id);
                SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
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
                "SQ[%d] HostCommandHandler:Processing:ECHO_CMD\r\n", sqw_idx);

            /* Construct and transmit response */
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id =
                DEV_OPS_API_MID_DEVICE_OPS_ECHO_RSP;
            rsp.response_info.rsp_hdr.size =
                sizeof(struct device_ops_echo_rsp_t) - sizeof(struct cmn_header_t);
            rsp.echo_payload = cmd->echo_payload;

            status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

            if(status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG,
                    "SQ[%d] HostCommandHandler:Pushed:ECHO_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                    sqw_idx, rsp.response_info.rsp_hdr.tag_id);
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR,
                          "SQ[%d]:HostCommandHandler:Tag_ID=%u:HostIface:Push:Failed\r\n",
                           sqw_idx, hdr->cmd_hdr.tag_id);
                SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
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
                "SQ[%d] HostCommandHandler:Processing:KERNEL_LAUNCH_CMD\r\n", sqw_idx);

            /* Compute Wait Cycles (cycles the command was sitting in SQ prior to launch)
               Snapshot current cycle */
            cycles.cmd_start_cycles = start_cycles;
            cycles.wait_cycles = (PMC_GET_LATENCY(start_cycles) & 0xFFFFFFF);
            cycles.exec_start_cycles = ((uint32_t)PMC_Get_Current_Cycles() & 0xFFFFFFFF);

            /* Blocking call to launch kernel */
            status = KW_Dispatch_Kernel_Launch_Cmd(cmd, sqw_idx, &kw_idx);

            if(status == STATUS_SUCCESS)
            {
                /* Create timeout for kernel_launch command to complete */
                /* TODO Add support in Device API Kernel Launch command to override timeout */
                /* cmd->timeout */
                sw_timer_idx = SW_Timer_Create_Timeout(&KW_Set_Abort_Status, kw_idx, KERNEL_LAUNCH_TIMEOUT(5));
                if(sw_timer_idx >= 0)
                {
                    /* Notify kernel worker to aggregate
                    kernel completion responses, and construct
                    and transmit command response to host
                    completion queue */
                    KW_Notify(kw_idx, &cycles, (uint8_t)sw_timer_idx);
                }
                else
                {
                    Log_Write(LOG_LEVEL_ERROR,
                        "SQ[%d] HostCommandHandler:CreateTimeot:Failed Tag ID:%d\r\n",
                         sqw_idx, cmd->command_info.cmd_hdr.tag_id);
                }
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "SQ[%d] HostCmdHdlr:KernelLaunch:Failed Tag ID=%d shire_mask:%lx Status:%d\r\n",
                     sqw_idx, cmd->command_info.cmd_hdr.tag_id, cmd->shire_mask, status);

                Log_Write(LOG_LEVEL_DEBUG,
                    "HostCmdHdlr:KernelLaunch:Failed CmdParam:code_start_address:%lx pointer_to_args:%lx\r\n",
                    cmd->code_start_address, cmd->pointer_to_args);

                /* Construct and transit command response */
                rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
                rsp.response_info.rsp_hdr.msg_id =
                    DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP;
                rsp.response_info.rsp_hdr.size =
                    sizeof(struct device_ops_kernel_launch_rsp_t) - sizeof(struct cmn_header_t);
                rsp.device_cmd_start_ts = start_cycles;
                rsp.device_cmd_wait_dur = cycles.wait_cycles;
                rsp.device_cmd_execute_dur = 0U;

                /* Populate the error type response */
                if (status == KW_ERROR_KERNEL_SHIRES_NOT_READY)
                {
                    rsp.status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_SHIRES_NOT_READY;
                }
                else if (status == KW_ERROR_KERNEL_INVALID_ADDRESS)
                {
                    rsp.status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_INVALID_ADDRESS;
                }
                else if (status == KW_ERROR_KERNEL_INVLD_ARGS_SIZE)
                {
                    rsp.status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_INVALID_ARGS_PAYLOAD_SIZE;
                }
                else
                {
                    rsp.status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_ERROR;
                }

                status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

                if(status == STATUS_SUCCESS)
                {
                    Log_Write(LOG_LEVEL_DEBUG,
                        "SQ[%d] HostCommandHandler:Pushed:KERNEL_LAUNCH_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                        sqw_idx, rsp.response_info.rsp_hdr.tag_id);
                }
                else
                {
                    Log_Write(LOG_LEVEL_ERROR,
                             "SQ[%d] HostCommandHandler:Tag_ID=%u:HostIface:Push:Failed\r\n",
                              sqw_idx, hdr->cmd_hdr.tag_id);
                    SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
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
                "SQ[%d] HostCommandHandler:Processing:KERNEL_ABORT_CMD\r\n", sqw_idx);

            /* Dispatch kernel abort command */
            status = KW_Dispatch_Kernel_Abort_Cmd(cmd, sqw_idx);

            if(status != STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "HostCmdHdlr:KernelAbort:Failed:Status:%d:CmdParams:kernel_launch_tag_id:%x\r\n",
                    status, cmd->kernel_launch_tag_id);

                /* Construct and transit command response */
                rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
                rsp.response_info.rsp_hdr.msg_id =
                    DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP;
                rsp.response_info.rsp_hdr.size =
                    sizeof(struct device_ops_kernel_abort_rsp_t) - sizeof(struct cmn_header_t);

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
                    Log_Write(LOG_LEVEL_ERROR,
                              "HostCommandHandler:Tag_ID=%u:HostIface:Push:Failed\r\n",
                               hdr->cmd_hdr.tag_id);
                    SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
                }

                /* Decrement commands count being processed by given SQW */
                SQW_Decrement_Command_Count(sqw_idx);
            }

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD:
        case DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_CMD:
        {
            struct device_ops_dma_readlist_cmd_t *cmd = (void *)hdr;
            struct device_ops_dma_readlist_rsp_t rsp;
            dma_flags_e dma_flag = DMA_NORMAL;
            dma_chan_id_e chan=DMA_CHAN_ID_INVALID;
            cm_iface_message_t cm_msg;
            status = STATUS_SUCCESS;
            uint64_t cm_shire_mask;
            uint64_t total_dma_size = 0;
            uint8_t dma_xfer_count;
            uint8_t loop_cnt;

            /* Design Notes: Note a DMA write command from host will
            trigger the implementation to configure a DMA read channel
            on device to move data from host to device, similarly a read
            command from host will trigger the implementation to configure
            a DMA write channel on device to move data from device to host */
            Log_Write(LOG_LEVEL_DEBUG,
                "SQ[%d] HostCommandHandler:Processing:DATA_READ_CMD\r\n", sqw_idx);

            /* If flags are set to extract MM and/or CM Trace buffers. Then only one DMA command will be processed. */
            if((cmd->command_info.cmd_hdr.flags & CMD_HEADER_FLAG_MM_TRACE_BUF) ||
               (cmd->command_info.cmd_hdr.flags & CMD_HEADER_FLAG_CM_TRACE_BUF))
            {
                dma_xfer_count = 1;
                /* If flags are set to extract both MM and CM Trace buffers. */
                if((cmd->command_info.cmd_hdr.flags & CMD_HEADER_FLAG_MM_TRACE_BUF) &&
                (cmd->command_info.cmd_hdr.flags & CMD_HEADER_FLAG_CM_TRACE_BUF))
                {
                    cm_shire_mask = Trace_Get_CM_Shire_Mask ();

                    if((cmd->list[TRACE_NODE_INDEX].size <= (MM_TRACE_BUFFER_SIZE + CM_TRACE_BUFFER_SIZE)) &&
                    (cm_shire_mask & CW_Get_Booted_Shires()) == cm_shire_mask)
                    {
                        /* Disable MM Trace.*/
                        Trace_RT_Control_MM(TRACE_DISABLE);

                        cm_msg.header.id = MM_TO_CM_MESSAGE_ID_TRACE_BUFFER_EVICT;

                        /* Send command to CM RT to disable Trace and evict Trace buffer. */
                        status = CM_Iface_Multicast_Send(cm_shire_mask, &cm_msg);

                        asm volatile("fence");
                        evict(to_Mem, (uint64_t *) MM_TRACE_BUFFER_BASE, MM_TRACE_BUFFER_SIZE);
                        WAIT_CACHEOPS;

                        cmd->list[TRACE_NODE_INDEX].src_device_phy_addr = MM_TRACE_BUFFER_BASE;
                        dma_flag = DMA_SOC_NO_BOUNDS_CHECK;
                    }
                    else
                    {
                        status = DMA_ERROR_OUT_OF_BOUNDS;
                    }
                }
                /* Check if flag is set to extract MM Trace buffer. */
                else if(cmd->command_info.cmd_hdr.flags & CMD_HEADER_FLAG_MM_TRACE_BUF)
                {
                    if(cmd->list[TRACE_NODE_INDEX].size <= MM_TRACE_BUFFER_SIZE)
                    {
                        /* Disable MM Trace.*/
                        Trace_RT_Control_MM(TRACE_DISABLE);

                        asm volatile("fence");
                        evict(to_Mem, (uint64_t *) MM_TRACE_BUFFER_BASE, MM_TRACE_BUFFER_SIZE);
                        WAIT_CACHEOPS;

                        cmd->list[TRACE_NODE_INDEX].src_device_phy_addr = MM_TRACE_BUFFER_BASE;
                        dma_flag = DMA_SOC_NO_BOUNDS_CHECK;
                    }
                    else
                    {
                        status = DMA_ERROR_OUT_OF_BOUNDS;
                    }
                }
                /* Check if flag is set to extract CM Trace buffer. */
                else if(cmd->command_info.cmd_hdr.flags & CMD_HEADER_FLAG_CM_TRACE_BUF)
                {
                    cm_shire_mask = Trace_Get_CM_Shire_Mask ();

                    if((cmd->list[TRACE_NODE_INDEX].size <= CM_TRACE_BUFFER_SIZE) &&
                    ((cm_shire_mask & CW_Get_Booted_Shires()) == cm_shire_mask))
                    {
                        cm_msg.header.id = MM_TO_CM_MESSAGE_ID_TRACE_BUFFER_EVICT;

                        /* Send command to CM RT to disable Trace and evict Trace buffer. */
                        status = CM_Iface_Multicast_Send(cm_shire_mask, &cm_msg);

                        cmd->list[TRACE_NODE_INDEX].src_device_phy_addr = CM_TRACE_BUFFER_BASE;
                        dma_flag = DMA_SOC_NO_BOUNDS_CHECK;
                    }
                    else
                    {
                        status = DMA_ERROR_OUT_OF_BOUNDS;
                    }
                }
            }
            else
            {
                /* Get number of transfer commands in the list, based on message payload length. */
                dma_xfer_count = (uint8_t)(cmd->command_info.cmd_hdr.size - DEVICE_CMD_HEADER_SIZE) /
                                           sizeof(struct dma_read_node);
            }

            if (status == STATUS_SUCCESS)
            {
                /* Obtain the next available DMA write channel */
                status = DMAW_Write_Find_Idle_Chan_And_Reserve(&chan, sqw_idx);
            }

            if(status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG, "SQ[%d] DMA_READ:channel_used:%d, dma xfer count=%d\r\n",
                    sqw_idx, chan, dma_xfer_count);

                for(loop_cnt=0; loop_cnt < dma_xfer_count; ++loop_cnt)
                {
                    Log_Write(LOG_LEVEL_DEBUG, "DMA_READ:src_device_phy_addr:%" PRIx64 "\r\n",
                        cmd->list[loop_cnt].src_device_phy_addr);
                    Log_Write(LOG_LEVEL_DEBUG, "DMA_READ:dst_host_phy_addr:%" PRIx64 "\r\n",
                        cmd->list[loop_cnt].dst_host_phy_addr);
                    Log_Write(LOG_LEVEL_DEBUG, "DMA_READ:size:%" PRIx32 "\r\n", cmd->list[loop_cnt].size);

                    total_dma_size += cmd->list[loop_cnt].size;
                }

                /* Compute Wait Cycles (cycles the command was sitting in SQ prior to launch)
                   Snapshot current cycle */
                cycles.cmd_start_cycles = start_cycles;
                cycles.wait_cycles = (PMC_GET_LATENCY(start_cycles) & 0xFFFFFFF);
                cycles.exec_start_cycles = ((uint32_t)PMC_Get_Current_Cycles() & 0xFFFFFFFF);

                /* Create timeout for DMA_Write command to complete */
                sw_timer_idx = SW_Timer_Create_Timeout(&DMAW_Write_Set_Abort_Status, chan,
                                    DMA_TIMEOUT_FACTOR(total_dma_size));

                if(sw_timer_idx >= 0)
                {
                    /* Initiate DMA write transfer */
                    status = DMAW_Write_Trigger_Transfer(chan, cmd, dma_xfer_count,
                        sqw_idx, &cycles, (uint8_t)sw_timer_idx, dma_flag);

                    if(status != STATUS_SUCCESS)
                    {
                        /* Free the registered SW Timeout */
                        SW_Timer_Cancel_Timeout((uint8_t)sw_timer_idx);
                    }
                }
                else
                {
                    Log_Write(LOG_LEVEL_ERROR,
                        "HostCommandHandler:CreateTimeot:Failed\r\n");
                }
            }

            if(status != STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "SQ[%d]:TID:%u:HostCmdHdlr:DMARead:Fail:%d\r\n",
                     sqw_idx, hdr->cmd_hdr.tag_id, status);

                for(loop_cnt=0; loop_cnt < dma_xfer_count; ++loop_cnt)
                {
                    Log_Write(LOG_LEVEL_ERROR,
                        "HostCmdHdlr:DMARead:Fail:TID:%u:src_device_phy_addr:%lx:size:%x\r\n",
                        hdr->cmd_hdr.tag_id, cmd->list[loop_cnt].src_device_phy_addr, cmd->list[loop_cnt].size);

                    Log_Write(LOG_LEVEL_DEBUG,
                        "HostCmdHdlr:DMARead:Fail:dst_host_virt_addr:%lx:dst_host_phy_addr:%lx\r\n",
                        cmd->list[loop_cnt].dst_host_virt_addr, cmd->list[loop_cnt].dst_host_phy_addr);
                }

                /* Construct and transmit command response */
                rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
                rsp.response_info.rsp_hdr.msg_id = (msg_id_t)(hdr->cmd_hdr.msg_id + 1U);
                /* TODO: SW-7137: Add dma_readlist_cmd handling
                rsp.response_info.rsp_hdr.msg_id =
                    DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP; */
                rsp.response_info.rsp_hdr.size = sizeof(rsp) - sizeof(struct cmn_header_t);
                /* Compute Wait Cycles (cycles the command was sitting
                in SQ prior to launch) Snapshot current cycle */
                rsp.device_cmd_start_ts = start_cycles;
                rsp.device_cmd_wait_dur = (PMC_GET_LATENCY(start_cycles) & 0xFFFFFFF);
                rsp.device_cmd_execute_dur = 0U;

                /* Populate the error type response */
                if (status == DMAW_ERROR_TIMEOUT_FIND_IDLE_CHANNEL)
                {
                    rsp.status = DEV_OPS_API_DMA_RESPONSE_TIMEOUT_IDLE_CHANNEL_UNAVAILABLE;
                }
                else if(status == DMA_ERROR_INVALID_ADDRESS)
                {
                    rsp.status = DEV_OPS_API_DMA_RESPONSE_INVALID_ADDRESS;
                }
                else if(status == DMA_ERROR_OUT_OF_BOUNDS)
                {
                    rsp.status = DEV_OPS_API_DMA_RESPONSE_INVALID_SIZE;
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
                             "HostCommandHandler:Tag_ID=%u:HostIface:Push:Failed\r\n",
                              hdr->cmd_hdr.tag_id);
                    SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
                }

                /* Decrement commands count being processed by given SQW */
                SQW_Decrement_Command_Count(sqw_idx);
            }

            break;
        }

        case DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD:
        case DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_CMD:
        {
            struct device_ops_dma_writelist_cmd_t *cmd = (void *)hdr;
            struct device_ops_dma_writelist_rsp_t rsp;
            dma_chan_id_e chan=DMA_CHAN_ID_INVALID;
            uint64_t total_dma_size=0;
            uint8_t dma_xfer_count;
            uint8_t loop_cnt;

            /* Design Notes: Note a DMA write command from host will trigger
            the implementation to configure a DMA read channel on device to move
            data from host to device, similarly a read command from host will
            trigger the implementation to configure a DMA write channel on device
            to move data from device to host */
            Log_Write(LOG_LEVEL_DEBUG,
                "SQ[%d] HostCommandHandler:Processing:DATA_WRITE_CMD\r\n", sqw_idx);

            /* Get number of transfer commands in the list, based on message payload length. */
            dma_xfer_count = (uint8_t)(cmd->command_info.cmd_hdr.size - DEVICE_CMD_HEADER_SIZE) /
                                       sizeof(struct dma_write_node);

            /* Obtain the next available DMA read channel */
            status = DMAW_Read_Find_Idle_Chan_And_Reserve(&chan, sqw_idx);

            if(status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG, "SQ[%d] DMA_WRITE:channel_used:%d, dma xfer count=%d \r\n",
                        sqw_idx, chan, dma_xfer_count);

                for(loop_cnt=0; loop_cnt < dma_xfer_count; ++loop_cnt)
                {
                    Log_Write(LOG_LEVEL_DEBUG, "DMA_WRITE:src_host_virt_addr:%" PRIx64 "\r\n",
                        cmd->list[loop_cnt].src_host_virt_addr);
                    Log_Write(LOG_LEVEL_DEBUG, "DMA_WRITE:src_host_phy_addr:%" PRIx64 "\r\n",
                        cmd->list[loop_cnt].src_host_phy_addr);
                    Log_Write(LOG_LEVEL_DEBUG, "DMA_WRITE:dst_device_phy_addr:%" PRIx64 "\r\n",
                        cmd->list[loop_cnt].dst_device_phy_addr);
                    Log_Write(LOG_LEVEL_DEBUG, "DMA_WRITE:size:%" PRIx32 "\r\n", cmd->list[loop_cnt].size);

                    total_dma_size += cmd->list[loop_cnt].size;
                }

                /* Compute Wait Cycles (cycles the command was sitting in SQ prior to launch)
                   Snapshot current cycle */
                cycles.cmd_start_cycles = start_cycles;
                cycles.wait_cycles = (PMC_GET_LATENCY(start_cycles) & 0xFFFFFFF);
                cycles.exec_start_cycles = ((uint32_t)PMC_Get_Current_Cycles() & 0xFFFFFFFF);

                /* Create timeout for DMA_Read command to complete */
                sw_timer_idx = SW_Timer_Create_Timeout(&DMAW_Read_Set_Abort_Status, chan,
                                        DMA_TIMEOUT_FACTOR(total_dma_size));

                if(sw_timer_idx >= 0)
                {
                    /* Initiate DMA read transfer */
                    status = DMAW_Read_Trigger_Transfer(chan, cmd, dma_xfer_count,
                        sqw_idx, &cycles, (uint8_t)sw_timer_idx);

                    if(status != STATUS_SUCCESS)
                    {
                        /* Free the registered SW Timeout */
                        SW_Timer_Cancel_Timeout((uint8_t)sw_timer_idx);
                    }
                }
                else
                {
                    Log_Write(LOG_LEVEL_ERROR,
                        "HostCommandHandler:Tag_ID=%u:CreateTimeot:Failed\r\n", hdr->cmd_hdr.tag_id);
                }
            }

            if(status != STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "HostCmdHdlr:DMAWrite:TID:%u:Fail:%d\r\n", hdr->cmd_hdr.tag_id, status);

                for(loop_cnt=0; loop_cnt < dma_xfer_count; ++loop_cnt)
                {
                    Log_Write(LOG_LEVEL_ERROR,
                        "HostCmdHdlr:DMAWrite:Fail:TID:%u:dst_device_phy_addr:%lx:size:%x\r\n",
                        hdr->cmd_hdr.tag_id, cmd->list[loop_cnt].dst_device_phy_addr, cmd->list[loop_cnt].size);

                    Log_Write(LOG_LEVEL_DEBUG,
                        "HostCmdHdlr:DMAWrite:Fail:src_host_virt_addr:%lx:src_host_phy_addr:%lx\r\n",
                        cmd->list[loop_cnt].src_host_virt_addr, cmd->list[loop_cnt].src_host_phy_addr);
                }

                /* Construct and transit command response */
                rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
                rsp.response_info.rsp_hdr.msg_id = (msg_id_t)(hdr->cmd_hdr.msg_id + 1U);
                /* TODO: SW-7137: Add dma_writelist_cmd handling
                rsp.response_info.rsp_hdr.msg_id =
                    DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP; */
                rsp.response_info.rsp_hdr.size = sizeof(rsp) - sizeof(struct cmn_header_t);
                /* Compute Wait Cycles (cycles the command was sitting
                in SQ prior to launch) Snapshot current cycle */
                rsp.device_cmd_start_ts = start_cycles;
                rsp.device_cmd_wait_dur = (PMC_GET_LATENCY(start_cycles) & 0xFFFFFFF);
                rsp.device_cmd_execute_dur = 0U;

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
                              "HostCommandHandler:Tag_ID=%u:HostIface:Push:Failed\r\n",
                               hdr->cmd_hdr.tag_id);
                    SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
                }

                /* Decrement commands count being processed by given SQW */
                SQW_Decrement_Command_Count(sqw_idx);
            }

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_CMD:
        {
            struct device_ops_trace_rt_control_cmd_t *cmd = (void *)hdr;
            struct device_ops_trace_rt_control_rsp_t rsp;
            status = STATUS_SUCCESS;

            Log_Write(LOG_LEVEL_DEBUG,
                "HostCommandHandler:Processing:TRACE_RT_CONTROL_CMD\r\n");

            /* Check if RT Component is MM Trace. */
            if(cmd->rt_type & TRACE_RT_CTRL_MM)
            {
                /* Check flag to Enable/Disable Trace. */
                if (cmd->control & (1U << 0))
                {
                    Trace_RT_Control_MM(TRACE_ENABLE);
                    Log_Write(LOG_LEVEL_DEBUG,
                            "TRACE_RT_CONTROL:MM:Trace Enabled.\r\n");
                }
                else
                {
                    Trace_RT_Control_MM(TRACE_DISABLE);
                    Log_Write(LOG_LEVEL_DEBUG,
                            "TRACE_RT_CONTROL:MM:Trace Disabled.\r\n");
                }

                /* Check flag to redirect logs to Trace or UART. */
                if (cmd->control & (1U << 1))
                {
                    Log_Set_Interface(LOG_DUMP_TO_TRACE);
                    Log_Write(LOG_LEVEL_CRITICAL,
                            "TRACE_RT_CONTROL:MM:Logs redirected to Trace buffer.\r\n");
                }
                else
                {
                    Log_Set_Interface(LOG_DUMP_TO_UART);
                    Log_Write(LOG_LEVEL_DEBUG,
                            "TRACE_RT_CONTROL:MM:Logs redirected to UART.\r\n");
                }
            }

            /* Check if RT Component is CM Trace. */
            if(cmd->rt_type & TRACE_RT_CTRL_CM)
            {
                mm_to_cm_message_trace_rt_control_t cm_msg;
                cm_msg.header.id = MM_TO_CM_MESSAGE_ID_TRACE_UPDATE_CONTROL;
                uint64_t cm_shire_mask;

                /* Check flag to Enable/Disable Trace. */
                if (cmd->control & (1U << 0))
                {
                    cm_msg.enable = TRACE_ENABLE;
                    Log_Write(LOG_LEVEL_DEBUG,
                            "TRACE_RT_CONTROL:CM:Enabling Trace.\r\n");
                }
                else
                {
                    cm_msg.enable = TRACE_DISABLE;
                    Log_Write(LOG_LEVEL_DEBUG,
                            "TRACE_RT_CONTROL:CM:Disabling Trace.\r\n");
                }

                /* Check flag to redirect logs to Trace or UART. */
                if (cmd->control & (1U << 1))
                {
                    cm_msg.log_interface = LOG_DUMP_TO_TRACE;
                    Log_Write(LOG_LEVEL_DEBUG,
                            "TRACE_RT_CONTROL:CM:Redirecting logs to Trace buffer.\r\n");
                }
                else
                {
                    cm_msg.log_interface = LOG_DUMP_TO_UART;
                    Log_Write(LOG_LEVEL_DEBUG,
                            "TRACE_RT_CONTROL:CM:Redirecting logs to UART.\r\n");
                }

                cm_shire_mask = Trace_Get_CM_Shire_Mask();

                if((cm_shire_mask & CW_Get_Booted_Shires()) == cm_shire_mask)
                {
                    /* Send command to CM RT to disable Trace and evict Trace buffer. */
                    status = CM_Iface_Multicast_Send(cm_shire_mask, (cm_iface_message_t*)&cm_msg);
                    if(status != STATUS_SUCCESS)
                    {
                        Log_Write(LOG_LEVEL_ERROR,
                            "TRACE_RT_CONTROL:CM:Tag_ID=%u:Failed to Enable/Disable Trace, and to redirect logs.\r\n",
                             hdr->cmd_hdr.tag_id);
                    }
                }
                else
                {
                    status = INVALID_CM_SHIRE_MASK;
                }
            }

            /* Construct and transmit command response. */
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id =
                DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_RSP;
            rsp.response_info.rsp_hdr.size = sizeof(rsp) - sizeof(struct cmn_header_t);

            /* Populate the response status */
            if(!(cmd->rt_type & TRACE_RT_CTRL_MM) || (cmd->rt_type & TRACE_RT_CTRL_CM))
            {
                rsp.status = DEV_OPS_TRACE_RT_CONTROL_RESPONSE_BAD_RT_TYPE;
            }
            if(status == STATUS_SUCCESS)
            {
                rsp.status = DEV_OPS_TRACE_RT_CONTROL_RESPONSE_SUCCESS;
            }
            else if(status == INVALID_CM_SHIRE_MASK)
            {
                rsp.status = DEV_OPS_TRACE_RT_CONTROL_RESPONSE_BAD_CONTROL_MASK;
            }
            else
            {
                rsp.status = DEV_OPS_TRACE_RT_CONTROL_RESPONSE_CM_RT_CTRL_ERROR;
            }

            status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

            if(status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG,
                    "HostCommandHandler:Pushed:TRACE_RT_CONTROL_RSP:tag_id=%x->Host_CQ\r\n",
                    rsp.response_info.rsp_hdr.tag_id);
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR,
                          "HostCommandHandler:Tag_ID=%u:HostIface:Push:Failed\r\n",
                           hdr->cmd_hdr.tag_id);
                SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
            }

            /* Decrement commands count being processed by given SQW */
            SQW_Decrement_Command_Count(sqw_idx);

            break;
        }
        default:
        {
            /* Decrement commands count being processed by given SQW */
            SQW_Decrement_Command_Count(sqw_idx);

            Log_Write(LOG_LEVEL_ERROR, "HostCmdHdlr:Tag_ID=%u:UnsupportedCmd\r\n",
                      hdr->cmd_hdr.tag_id);
            status = -1;
            break;
        }
    }

    return status;
}
