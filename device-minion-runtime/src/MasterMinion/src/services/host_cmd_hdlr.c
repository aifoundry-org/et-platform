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
*       compatibility_cmd_handler
*
*   DESCRIPTION
*
*       Process host compatibility command, and transmit response.
*
*   INPUTS
*
*       command_buffer   Buffer containing command to process
*       sqw_idx          Submission queue index
*
*   OUTPUTS
*
*       int8_t           Successful status or error code.
*
***********************************************************************/
static inline int8_t compatibility_cmd_handler(void* command_buffer, uint8_t sqw_idx)
{
    const struct device_ops_echo_cmd_t *cmd = (struct device_ops_echo_cmd_t *)command_buffer;
    struct device_ops_api_compatibility_rsp_t rsp;
    int8_t status = STATUS_SUCCESS;

    Log_Write(LOG_LEVEL_DEBUG,
        "SQ[%d] HostCommandHandler:Processing:API_COMPATIBILITY_CMD\r\n",sqw_idx);

    /* Construct and transmit response */
    rsp.response_info.rsp_hdr.tag_id = cmd->command_info.cmd_hdr.tag_id;
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
                    sqw_idx, cmd->command_info.cmd_hdr.tag_id);
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
    }

    /* Decrement commands count being processed by given SQW */
    SQW_Decrement_Command_Count(sqw_idx);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       fw_version_cmd_handler
*
*   DESCRIPTION
*
*       Process host firmware version command, and transmit response.
*
*   INPUTS
*
*       command_buffer   Buffer containing command to process
*       sqw_idx          Submission queue index
*
*   OUTPUTS
*
*       int8_t           Successful status or error code.
*
***********************************************************************/
static inline int8_t fw_version_cmd_handler(void* command_buffer, uint8_t sqw_idx)
{
    const struct device_ops_device_fw_version_cmd_t *cmd =
        (struct device_ops_device_fw_version_cmd_t *)command_buffer;
    struct device_ops_fw_version_rsp_t rsp = { 0 };
    int8_t status = STATUS_SUCCESS;
    mm2sp_fw_type_e fw_type = 0;

    Log_Write(LOG_LEVEL_DEBUG,
        "SQ[%d] HostCommandHandler:Processing:FW_VERSION_CMD\r\n", sqw_idx);

    /* Construct and transmit response */
    rsp.response_info.rsp_hdr.tag_id = cmd->command_info.cmd_hdr.tag_id;
    rsp.response_info.rsp_hdr.msg_id =
        DEV_OPS_API_MID_DEVICE_OPS_FW_VERSION_RSP;
    rsp.response_info.rsp_hdr.size =
        sizeof(struct device_ops_fw_version_rsp_t) - sizeof(struct cmn_header_t);

    if (cmd->firmware_type == DEV_OPS_FW_TYPE_MASTER_MINION_FW)
    {
        fw_type = MM2SP_MASTER_MINION_FW;
        rsp.type = DEV_OPS_FW_TYPE_MASTER_MINION_FW;
    }
    else if (cmd->firmware_type == DEV_OPS_FW_TYPE_MACHINE_MINION_FW)
    {
        fw_type = MM2SP_MACHINE_MINION_FW;
        rsp.type = DEV_OPS_FW_TYPE_MACHINE_MINION_FW;
    }
    else if (cmd->firmware_type == DEV_OPS_FW_TYPE_WORKER_MINION_FW)
    {
        fw_type = MM2SP_WORKER_MINION_FW;
        rsp.type = DEV_OPS_FW_TYPE_WORKER_MINION_FW;
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR,
            "SQ[%d] HostCommandHandler:FW_VERSION_CMD:Invalid FW type received from host\r\n",
            sqw_idx);
        status = GENERAL_ERROR;
    }

    if(status == STATUS_SUCCESS)
    {
        /* Request SP for FW version */
        status = SP_Iface_Get_Fw_Version(fw_type, (uint8_t*)&rsp.major, (uint8_t*)&rsp.minor,
            (uint8_t*)&rsp.patch);

        if(status != STATUS_SUCCESS)
        {
            /* Reset values */
            rsp.major = 0;
            rsp.minor = 0;
            rsp.patch = 0;

            Log_Write(LOG_LEVEL_ERROR,
                "SQ[%d] HostCommandHandler:FW_VERSION_CMD:Request to SP failed:%d\r\n",
                sqw_idx, status);
        }
    }

    /* Push response to Host */
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
                                    sqw_idx, cmd->command_info.cmd_hdr.tag_id);
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
    }

    /* Decrement commands count being processed by given SQW */
    SQW_Decrement_Command_Count(sqw_idx);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       echo_cmd_handler
*
*   DESCRIPTION
*
*       Process host echo command, and transmit response.
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
static inline int8_t echo_cmd_handler(void* command_buffer, uint8_t sqw_idx,
    uint64_t start_cycles)
{
    const struct device_ops_echo_cmd_t *cmd =
        (struct device_ops_echo_cmd_t *)command_buffer;
    struct device_ops_echo_rsp_t rsp;
    int8_t status = STATUS_SUCCESS;

    Log_Write(LOG_LEVEL_DEBUG,
        "SQ[%d] HostCommandHandler:Processing:ECHO_CMD\r\n", sqw_idx);

    /* Construct and transmit response */
    rsp.response_info.rsp_hdr.tag_id = cmd->command_info.cmd_hdr.tag_id;
    rsp.response_info.rsp_hdr.msg_id =
        DEV_OPS_API_MID_DEVICE_OPS_ECHO_RSP;
    rsp.response_info.rsp_hdr.size =
        sizeof(struct device_ops_echo_rsp_t) - sizeof(struct cmn_header_t);
    rsp.device_cmd_start_ts = start_cycles;

    status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

    if(status == STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_DEBUG,
            "SQ[%d] HostCommandHandler:Pushed:ECHO_CMD_RSP:tag_id=%x->Host_CQ\r\n",
            sqw_idx, rsp.response_info.rsp_hdr.tag_id);
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "SQ[%d]:HostCommandHandler:Tag_ID=%u:HostIface:Push:Failed\r\n",
                    sqw_idx, cmd->command_info.cmd_hdr.tag_id);
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
    }

    /* Decrement commands count being processed by given SQW */
    SQW_Decrement_Command_Count(sqw_idx);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       kernel_launch_cmd_handler
*
*   DESCRIPTION
*
*       Process host kernel launch command, and transmit response as needed
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
static inline int8_t kernel_launch_cmd_handler(void* command_buffer, uint8_t sqw_idx,
    uint64_t start_cycles)
{
    struct device_ops_kernel_launch_cmd_t *cmd =
        (struct device_ops_kernel_launch_cmd_t *)command_buffer;
    struct device_ops_kernel_launch_rsp_t rsp;
    uint8_t kw_idx;
    exec_cycles_t cycles;
    int8_t status = STATUS_SUCCESS;

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
        /* Notify kernel worker to aggregate kernel completion responses, and construct
        and transmit command response to host completion queue */
        KW_Notify(kw_idx, &cycles);

        Log_Write(LOG_LEVEL_DEBUG,
            "SQ[%d] HostCommandHandler:KW:%d:Notified\r\n", sqw_idx, kw_idx);
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
        rsp.response_info.rsp_hdr.tag_id = cmd->command_info.cmd_hdr.tag_id;
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
                        sqw_idx, cmd->command_info.cmd_hdr.tag_id);
            SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
        }

        /* Decrement commands count being processed by given SQW */
        SQW_Decrement_Command_Count(sqw_idx);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       kernel_abort_cmd_handler
*
*   DESCRIPTION
*
*       Process host kernel abort command, and transmit response as needed
*
*   INPUTS
*
*       command_buffer   Buffer containing command to process
*       sqw_idx          Submission queue index
*
*   OUTPUTS
*
*       int8_t           Successful status or error code.
*
***********************************************************************/
static inline int8_t kernel_abort_cmd_handler(void* command_buffer, uint8_t sqw_idx)
{
    struct device_ops_kernel_abort_cmd_t *cmd =
        (struct device_ops_kernel_abort_cmd_t *)command_buffer;
    struct device_ops_kernel_abort_rsp_t rsp;
    int8_t status = STATUS_SUCCESS;

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
        rsp.response_info.rsp_hdr.tag_id = cmd->command_info.cmd_hdr.tag_id;
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
                        cmd->command_info.cmd_hdr.tag_id);
            SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
        }

        /* Decrement commands count being processed by given SQW */
        SQW_Decrement_Command_Count(sqw_idx);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       dma_readlist_cmd_process_trace_flags
*
*   DESCRIPTION
*
*       Process host command special flags for DMA read requests.
*
*   INPUTS
*
*       cmd              Buffer containing command to process
*
*   OUTPUTS
*
*       int8_t           Successful status or error code.
*
***********************************************************************/
static inline int8_t dma_readlist_cmd_process_trace_flags(struct device_ops_dma_readlist_cmd_t *cmd)
{
    int8_t status = STATUS_SUCCESS;
    uint64_t cm_shire_mask;
    cm_iface_message_t cm_msg;

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

            ETSOC_MEM_EVICT((uint64_t *)MM_TRACE_BUFFER_BASE, MM_TRACE_BUFFER_SIZE, to_Mem)

            cmd->list[TRACE_NODE_INDEX].src_device_phy_addr = MM_TRACE_BUFFER_BASE;
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

            ETSOC_MEM_EVICT((uint64_t *)MM_TRACE_BUFFER_BASE, MM_TRACE_BUFFER_SIZE, to_Mem)

            cmd->list[TRACE_NODE_INDEX].src_device_phy_addr = MM_TRACE_BUFFER_BASE;
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
        }
        else
        {
            status = DMA_ERROR_OUT_OF_BOUNDS;
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       dma_readlist_cmd_handler
*
*   DESCRIPTION
*
*       Process host DMA read list command, and transmit response as needed
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
static inline int8_t dma_readlist_cmd_handler(void* command_buffer, uint8_t sqw_idx,
    uint64_t start_cycles)
{
    struct device_ops_dma_readlist_cmd_t *cmd =
        (struct device_ops_dma_readlist_cmd_t *)command_buffer;
    struct device_ops_dma_readlist_rsp_t rsp;
    dma_flags_e dma_flag;
    dma_write_chan_id_e chan = DMA_CHAN_ID_WRITE_INVALID;
    int8_t status = STATUS_SUCCESS;
    uint64_t total_dma_size = 0;
    uint8_t dma_xfer_count;
    uint8_t loop_cnt;
    exec_cycles_t cycles;
    int8_t sw_timer_idx = -1;

    /* Design Notes: Note a DMA write command from host will
    trigger the implementation to configure a DMA read channel
    on device to move data from host to device, similarly a read
    command from host will trigger the implementation to configure
    a DMA write channel on device to move data from device to host */
    Log_Write(LOG_LEVEL_DEBUG,
        "SQ[%d] HostCommandHandler:Processing:DATA_READ_CMD\r\n", sqw_idx);

    /* Check if no special flag is set. */
    if((cmd->command_info.cmd_hdr.flags & CMD_HEADER_FLAG_MM_TRACE_BUF) ||
        (cmd->command_info.cmd_hdr.flags & CMD_HEADER_FLAG_CM_TRACE_BUF))
    {
        dma_xfer_count = 1;
        dma_flag = DMA_SOC_NO_BOUNDS_CHECK;
        status = dma_readlist_cmd_process_trace_flags(cmd);
    }
    else
    {
        dma_flag = DMA_NORMAL;
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
            DMA_TRANSFER_TIMEOUT);

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
                "HostCommandHandler:CreateTimeout:Failed\r\n");
        }
    }

    if(status != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR,
            "SQ[%d]:TID:%u:HostCmdHdlr:DMARead:Fail:%d\r\n",
                sqw_idx, cmd->command_info.cmd_hdr.tag_id, status);

        for(loop_cnt=0; loop_cnt < dma_xfer_count; ++loop_cnt)
        {
            Log_Write(LOG_LEVEL_ERROR,
                "HostCmdHdlr:DMARead:Fail:TID:%u:src_device_phy_addr:%lx:size:%x\r\n",
                cmd->command_info.cmd_hdr.tag_id, cmd->list[loop_cnt].src_device_phy_addr,
                cmd->list[loop_cnt].size);

            Log_Write(LOG_LEVEL_DEBUG,
                "HostCmdHdlr:DMARead:Fail:dst_host_virt_addr:%lx:dst_host_phy_addr:%lx\r\n",
                cmd->list[loop_cnt].dst_host_virt_addr, cmd->list[loop_cnt].dst_host_phy_addr);
        }

        /* Construct and transmit command response */
        rsp.response_info.rsp_hdr.tag_id = cmd->command_info.cmd_hdr.tag_id;
        rsp.response_info.rsp_hdr.msg_id = (msg_id_t)(cmd->command_info.cmd_hdr.msg_id + 1U);
        /* TODO: SW-7137: Add dma_readlist_cmd handling
        rsp.response_info.rsp_hdr.msg_id =
            DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP */
        rsp.response_info.rsp_hdr.size = sizeof(rsp) - sizeof(struct cmn_header_t);
        /* Compute Wait Cycles (cycles the command was sitting
        in SQ prior to launch) Snapshot current cycle */
        rsp.device_cmd_start_ts = start_cycles;
        rsp.device_cmd_wait_dur = (PMC_GET_LATENCY(start_cycles) & 0xFFFFFFF);
        rsp.device_cmd_execute_dur = 0U;

        /* Populate the error type response */
        switch (status)
        {
            case DMAW_ERROR_TIMEOUT_FIND_IDLE_CHANNEL:
                rsp.status = DEV_OPS_API_DMA_RESPONSE_TIMEOUT_IDLE_CHANNEL_UNAVAILABLE;
                break;
            case DMA_ERROR_INVALID_ADDRESS:
                rsp.status = DEV_OPS_API_DMA_RESPONSE_INVALID_ADDRESS;
                break;
            case DMA_ERROR_OUT_OF_BOUNDS:
                rsp.status = DEV_OPS_API_DMA_RESPONSE_INVALID_SIZE;
                break;
            default:
                rsp.status = DEV_OPS_API_DMA_RESPONSE_ERROR;
                break;
        }

        Log_Write(LOG_LEVEL_DEBUG,
            "HostCommandHandler:Pushing:DATA_READ_CMD_RSP:tag_id=%x->Host_CQ\r\n",
            rsp.response_info.rsp_hdr.tag_id);

        status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

        if(status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR, "HostCommandHandler:Tag_ID=%u:HostIface:Push:Failed\r\n",
                              cmd->command_info.cmd_hdr.tag_id);
            SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
        }
        /* Decrement commands count being processed by given SQW */
        SQW_Decrement_Command_Count(sqw_idx);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       dma_writelist_cmd_handler
*
*   DESCRIPTION
*
*       Process host DMA write list command, and transmit response as needed
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
static inline int8_t dma_writelist_cmd_handler(void* command_buffer, uint8_t sqw_idx,
    uint64_t start_cycles)
{
    const struct device_ops_dma_writelist_cmd_t *cmd =
        (struct device_ops_dma_writelist_cmd_t *)command_buffer;
    struct device_ops_dma_writelist_rsp_t rsp;
    dma_read_chan_id_e chan = DMA_CHAN_ID_READ_INVALID;
    uint64_t total_dma_size=0;
    uint8_t dma_xfer_count;
    uint8_t loop_cnt;
    int8_t status = STATUS_SUCCESS;
    int8_t sw_timer_idx = -1;
    exec_cycles_t cycles;

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
            DMA_TRANSFER_TIMEOUT);

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
            Log_Write(LOG_LEVEL_ERROR, "HostCommandHandler:Tag_ID=%u:CreateTimeout:Failed\r\n",
                cmd->command_info.cmd_hdr.tag_id);
        }
    }

    if(status != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "HostCmdHdlr:DMAWrite:TID:%u:Fail:%d\r\n",
                  cmd->command_info.cmd_hdr.tag_id, status);

        for(loop_cnt=0; loop_cnt < dma_xfer_count; ++loop_cnt)
        {
            Log_Write(LOG_LEVEL_ERROR,
                "HostCmdHdlr:DMAWrite:Fail:TID:%u:dst_device_phy_addr:%lx:size:%x\r\n",
                cmd->command_info.cmd_hdr.tag_id, cmd->list[loop_cnt].dst_device_phy_addr,
                cmd->list[loop_cnt].size);

            Log_Write(LOG_LEVEL_DEBUG,
                "HostCmdHdlr:DMAWrite:Fail:src_host_virt_addr:%lx:src_host_phy_addr:%lx\r\n",
                cmd->list[loop_cnt].src_host_virt_addr, cmd->list[loop_cnt].src_host_phy_addr);
        }

        /* Construct and transit command response */
        rsp.response_info.rsp_hdr.tag_id = cmd->command_info.cmd_hdr.tag_id;
        rsp.response_info.rsp_hdr.msg_id = (msg_id_t)(cmd->command_info.cmd_hdr.msg_id + 1U);
        /* TODO: SW-7137: Add dma_writelist_cmd handling
        rsp.response_info.rsp_hdr.msg_id =
            DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP */
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
            Log_Write(LOG_LEVEL_ERROR, "HostCommandHandler:Tag_ID=%u:HostIface:Push:Failed\r\n",
                        cmd->command_info.cmd_hdr.tag_id);
            SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
        }

        /* Decrement commands count being processed by given SQW */
        SQW_Decrement_Command_Count(sqw_idx);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       trace_rt_control_cmd_handler
*
*   DESCRIPTION
*
*       Process host Trace runtime control command, and transmit response.
*
*   INPUTS
*
*       command_buffer   Buffer containing command to process
*       sqw_idx          Submission queue index
*
*   OUTPUTS
*
*       int8_t           Successful status or error code.
*
***********************************************************************/
static inline int8_t trace_rt_control_cmd_handler(void* command_buffer, uint8_t sqw_idx)
{
    const struct device_ops_trace_rt_control_cmd_t *cmd =
        (struct device_ops_trace_rt_control_cmd_t *)command_buffer;
    struct device_ops_trace_rt_control_rsp_t rsp;
    int8_t status = STATUS_SUCCESS;

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
            Log_Set_Interface(LOG_DUMP_TO_UART);
            Log_Write(LOG_LEVEL_DEBUG,
                    "TRACE_RT_CONTROL:MM:Logs redirected to UART.\r\n");
        }
        else
        {
            Log_Set_Interface(LOG_DUMP_TO_TRACE);
            Log_Write(LOG_LEVEL_CRITICAL,
                    "TRACE_RT_CONTROL:MM:Logs redirected to Trace buffer.\r\n");
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
            cm_msg.log_interface = LOG_DUMP_TO_UART;
            Log_Write(LOG_LEVEL_DEBUG,
                    "TRACE_RT_CONTROL:CM:Redirecting logs to UART.\r\n");
        }
        else
        {
            cm_msg.log_interface = LOG_DUMP_TO_TRACE;
            Log_Write(LOG_LEVEL_DEBUG,
                    "TRACE_RT_CONTROL:CM:Redirecting logs to Trace buffer.\r\n");
        }

        cm_shire_mask = Trace_Get_CM_Shire_Mask();

        if((cm_shire_mask & CW_Get_Booted_Shires()) != cm_shire_mask)
        {
            status = INVALID_CM_SHIRE_MASK;
        }
        else
        {
           /* Send command to CM RT to disable Trace and evict Trace buffer. */
            status = CM_Iface_Multicast_Send(cm_shire_mask, (cm_iface_message_t*)&cm_msg);
        }

        if(status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR,
            "TRACE_RT_CONTROL:CM:Tag_ID=%u:Failed to Enable/Disable Trace, and to redirect logs.\r\n",
                cmd->command_info.cmd_hdr.tag_id);
        }
    }

    /* Construct and transmit command response. */
    rsp.response_info.rsp_hdr.tag_id = cmd->command_info.cmd_hdr.tag_id;
    rsp.response_info.rsp_hdr.msg_id =
        DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_RSP;
    rsp.response_info.rsp_hdr.size = sizeof(rsp) - sizeof(struct cmn_header_t);

    /* Populate the response status */
    if(!((cmd->rt_type & TRACE_RT_CTRL_MM) || (cmd->rt_type & TRACE_RT_CTRL_CM)))
    {
        rsp.status = DEV_OPS_TRACE_RT_CONTROL_RESPONSE_BAD_RT_TYPE;
    }
    else if(status == STATUS_SUCCESS)
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

    Log_Write(LOG_LEVEL_DEBUG,
        "HostCommandHandler:Pushing:TRACE_RT_CONTROL_RSP:tag_id=%x->Host_CQ\r\n",
        rsp.response_info.rsp_hdr.tag_id);

    status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

    if(status != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "HostCommandHandler:Tag_ID=%u:HostIface:Push:Failed\r\n",
                        cmd->command_info.cmd_hdr.tag_id);
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
    }

    /* Decrement commands count being processed by given SQW */
    SQW_Decrement_Command_Count(sqw_idx);

    return status;
}

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
    const struct cmd_header_t *hdr = command_buffer;

    switch (hdr->cmd_hdr.msg_id)
    {
        case DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD:
            status = compatibility_cmd_handler(command_buffer, sqw_idx);
            break;
        case DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD:
            status = fw_version_cmd_handler(command_buffer, sqw_idx);
            break;
        case DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD:
            status = echo_cmd_handler(command_buffer, sqw_idx, start_cycles);
            break;
        case DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD:
            status = kernel_launch_cmd_handler(command_buffer, sqw_idx, start_cycles);
            break;
        case DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_CMD:
            status = kernel_abort_cmd_handler(command_buffer, sqw_idx);
            break;
        case DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD:
        case DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_CMD:
            status = dma_readlist_cmd_handler(command_buffer, sqw_idx, start_cycles);
            break;
        case DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD:
        case DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_CMD:
            status = dma_writelist_cmd_handler(command_buffer, sqw_idx, start_cycles);
            break;
        case DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_CMD:
            status = trace_rt_control_cmd_handler(command_buffer, sqw_idx);
            break;
        default:
            /* Decrement commands count being processed by given SQW */
            SQW_Decrement_Command_Count(sqw_idx);

            Log_Write(LOG_LEVEL_ERROR, "HostCmdHdlr:Tag_ID=%u:UnsupportedCmd\r\n",
                      hdr->cmd_hdr.tag_id);
            status = -1;
            break;
    }

    return status;
}
