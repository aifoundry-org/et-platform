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
        Host_HP_Command_Handler
*/
/***********************************************************************/
/* mm_et_svcs */
#include <etsoc/drivers/pmu/pmu.h>
#include <etsoc/isa/cacheops.h>

/* mm specific headers */
#include "services/host_cmd_hdlr.h"
#include "services/host_iface.h"
#include "services/log.h"
#include "services/trace.h"
#include "services/cm_iface.h"
#include "services/sp_iface.h"
#include "workers/kw.h"
#include "workers/cw.h"
#include "workers/dmaw.h"
#include "workers/sqw.h"
#include "workers/sqw_hp.h"
#include "config/mm_config.h"

/* mm_rt_helpers */
#include "layout.h"

/*! \def TRACE_NODE_INDEX
    \brief Default trace node index in DMA list command when flag is set to extract Trace buffers.
           As flags are per command, not per transfer in the command, that is why when Trace flags
           are set only one transfer will be done which is the first one.
*/
#define TRACE_NODE_INDEX 0

/*! \def DMA_TO_DEVICEAPI_STATUS
    \brief Helper macro to convert DMA Error to DEVICE API Errors
*/
#define DMA_TO_DEVICEAPI_STATUS(status, abort_status, ret_status)                                  \
    if ((status == DMAW_ABORTED_IDLE_CHANNEL_SEARCH) || (abort_status == HOST_CMD_STATUS_ABORTED)) \
    {                                                                                              \
        ret_status = DEV_OPS_API_DMA_RESPONSE_HOST_ABORTED;                                        \
    }                                                                                              \
    else if (status == DMA_ERROR_INVALID_ADDRESS)                                                  \
    {                                                                                              \
        ret_status = DEV_OPS_API_DMA_RESPONSE_INVALID_ADDRESS;                                     \
    }                                                                                              \
    else if ((status == DMA_ERROR_OUT_OF_BOUNDS) || (status == DMA_ERROR_INVALID_XFER_COUNT))      \
    {                                                                                              \
        ret_status = DEV_OPS_API_DMA_RESPONSE_INVALID_SIZE;                                        \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
        ret_status = DEV_OPS_API_DMA_RESPONSE_ERROR;                                               \
    }

/************************************************************************
*
*   FUNCTION
*
*       abort_cmd_handler
*
*   DESCRIPTION
*
*       Process host abort command, and transmit response.
*
*   INPUTS
*
*       command_buffer   Buffer containing command to process
*       sqw_hp_idx       HP Submission queue index
*
*   OUTPUTS
*
*       int8_t           Successful status or error code.
*
***********************************************************************/
static inline int8_t abort_cmd_handler(void *command_buffer, uint8_t sqw_hp_idx)
{
    const struct device_ops_abort_cmd_t *cmd = (struct device_ops_abort_cmd_t *)command_buffer;
    struct device_ops_abort_rsp_t rsp;
    int8_t status = STATUS_SUCCESS;

    TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_ABORT_CMD, sqw_hp_idx,
        cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_RECEIVED)

    Log_Write(LOG_LEVEL_DEBUG, "SQ_HP[%d] abort_cmd_handler:Processing:ABORT_CMD\r\n", sqw_hp_idx);

    /* Construct and transmit response */
    rsp.response_info.rsp_hdr.tag_id = cmd->command_info.cmd_hdr.tag_id;
    rsp.response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_ABORT_RSP;
    /* Set the abort command status */
    rsp.status = DEV_OPS_API_ABORT_RESPONSE_SUCCESS;

    TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_ABORT_CMD, sqw_hp_idx,
        cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_EXECUTING)

    /* TODO: tag_id based abort needs to be implemented */

    /* Commands abort will be done in 3 phases:
    1. Abort any pending commands in a particular SQ
    2. Abort any dispatched DMA read/write commands for the particular SQ
    3. Abort any dispatched Kernel command for the particular SQ */

    /* Blocking call that aborts all pending commands in the paired normal SQ */
    SQW_Abort_All_Pending_Commands(sqw_hp_idx);

    /* Blocking call that aborts all DMA read channels */
    DMAW_Abort_All_Dispatched_Read_Channels(sqw_hp_idx);

    /* Blocking call that aborts all DMA write channels */
    DMAW_Abort_All_Dispatched_Write_Channels(sqw_hp_idx);

    /* Blocking call that aborts all dispatched kernels */
    KW_Abort_All_Dispatched_Kernels(sqw_hp_idx);

    Log_Write(LOG_LEVEL_DEBUG, "SQ_HP[%d] abort_cmd_handler:ABORT_CMD processing complete\r\n",
        sqw_hp_idx);

#if TEST_FRAMEWORK
    /* For SP2MM command response, we need to provide the total size = header + payload */
    rsp.response_info.rsp_hdr.size = sizeof(struct device_ops_abort_rsp_t);
    status = SP_Iface_Push_Rsp_To_SP2MM_CQ(&rsp, sizeof(rsp));
#else
    rsp.response_info.rsp_hdr.size =
        sizeof(struct device_ops_abort_rsp_t) - sizeof(struct cmn_header_t);
    status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));
#endif

    if (status == STATUS_SUCCESS)
    {
        TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_ABORT_CMD, sqw_hp_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_SUCCEEDED)

        Log_Write(LOG_LEVEL_DEBUG,
            "SQ_HP[%d] abort_cmd_handler:Pushed:ABORT response:tag_id=%x->Host_CQ\r\n", sqw_hp_idx,
            rsp.response_info.rsp_hdr.tag_id);
    }
    else
    {
        TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_ABORT_CMD, sqw_hp_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_FAILED)

        Log_Write(LOG_LEVEL_ERROR,
            "SQ_HP[%d] abort_cmd_handler:Tag_ID=%u:HostIface:Push:Failed\r\n", sqw_hp_idx,
            cmd->command_info.cmd_hdr.tag_id);

        SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
    }
#if !TEST_FRAMEWORK
    /* Decrement the SQW HP count */
    SQW_HP_Decrement_Command_Count(sqw_hp_idx);
#endif
    return status;
}

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
static inline int8_t compatibility_cmd_handler(void *command_buffer, uint8_t sqw_idx)
{
    const struct check_device_ops_api_compatibility_cmd_t *cmd =
        (struct check_device_ops_api_compatibility_cmd_t *)command_buffer;
    struct device_ops_api_compatibility_rsp_t rsp;
    int8_t status = STATUS_SUCCESS;

    TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD, sqw_idx,
        cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_RECEIVED)

    Log_Write(
        LOG_LEVEL_DEBUG, "SQ[%d] HostCommandHandler:Processing:API_COMPATIBILITY_CMD\r\n", sqw_idx);

    /* Construct and transmit response */
    rsp.response_info.rsp_hdr.tag_id = cmd->command_info.cmd_hdr.tag_id;
    rsp.response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP;
    rsp.response_info.rsp_hdr.size =
        sizeof(struct device_ops_api_compatibility_rsp_t) - sizeof(struct cmn_header_t);
    rsp.status = DEV_OPS_API_COMPATIBILITY_RESPONSE_SUCCESS;

    /* Get the SQW state to check for command abort */
    if (SQW_Get_State(sqw_idx) == SQW_STATE_ABORTED)
    {
        rsp.status = DEV_OPS_API_COMPATIBILITY_RESPONSE_HOST_ABORTED;
    }
    else
    {
        TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD, sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_EXECUTING)

        rsp.major = DEVICE_OPS_API_MAJOR;
        rsp.minor = DEVICE_OPS_API_MINOR;
        rsp.patch = DEVICE_OPS_API_PATCH;

        /* Validate if Host software version is compatible with the device software */
        if (cmd->major != DEVICE_OPS_API_MAJOR)
        {
            rsp.status = DEV_OPS_API_COMPATIBILITY_RESPONSE_INCOMPATIBLE_MAJOR;
        }
        else if (cmd->minor != DEVICE_OPS_API_MINOR)
        {
            rsp.status = DEV_OPS_API_COMPATIBILITY_RESPONSE_INCOMPATIBLE_MINOR;
        }
        else if (cmd->patch != DEVICE_OPS_API_PATCH)
        {
            rsp.status = DEV_OPS_API_COMPATIBILITY_RESPONSE_INCOMPATIBLE_PATCH;
        }
    }

    status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

    if (status == STATUS_SUCCESS)
    {
        /* Check for command status for trace logging */
        if (rsp.status == DEV_OPS_API_COMPATIBILITY_RESPONSE_SUCCESS)
        {
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_SUCCEEDED)
        }
        else if (rsp.status == DEV_OPS_API_COMPATIBILITY_RESPONSE_HOST_ABORTED)
        {
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_ABORTED)
        }
        else
        {
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_FAILED)
        }

        Log_Write(LOG_LEVEL_DEBUG,
            "SQ[%d] HostCommandHandler:Pushed:API_COMPATIBILITY_CMD_RSP:tag_id=%x->Host_CQ\r\n",
            sqw_idx, rsp.response_info.rsp_hdr.tag_id);
    }
    else
    {
        TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD, sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_FAILED)

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
static inline int8_t fw_version_cmd_handler(void *command_buffer, uint8_t sqw_idx)
{
    const struct device_ops_device_fw_version_cmd_t *cmd =
        (struct device_ops_device_fw_version_cmd_t *)command_buffer;
    struct device_ops_fw_version_rsp_t rsp = { 0 };
    int8_t status = STATUS_SUCCESS;
    mm2sp_fw_type_e fw_type = 0;

    TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD, sqw_idx,
        cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_RECEIVED)

    Log_Write(LOG_LEVEL_DEBUG, "SQ[%d] HostCommandHandler:Processing:FW_VERSION_CMD\r\n", sqw_idx);

    /* Construct and transmit response */
    rsp.response_info.rsp_hdr.tag_id = cmd->command_info.cmd_hdr.tag_id;
    rsp.response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_FW_VERSION_RSP;
    rsp.status = DEV_OPS_API_FW_VERSION_RESPONSE_SUCCESS;

    /* Get the SQW state to check for command abort */
    if (SQW_Get_State(sqw_idx) == SQW_STATE_ABORTED)
    {
        rsp.status = DEV_OPS_API_FW_VERSION_RESPONSE_HOST_ABORTED;
    }
    else
    {
        TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD, sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_EXECUTING)

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
            rsp.status = DEV_OPS_API_FW_VERSION_RESPONSE_BAD_FW_TYPE;
            status = GENERAL_ERROR;
        }

        if (status == STATUS_SUCCESS)
        {
            /* Request SP for FW version */
            status = SP_Iface_Get_Fw_Version(
                fw_type, (uint8_t *)&rsp.major, (uint8_t *)&rsp.minor, (uint8_t *)&rsp.patch);

            if (status != STATUS_SUCCESS)
            {
                rsp.status = DEV_OPS_API_FW_VERSION_RESPONSE_NOT_AVAILABLE;

                /* Reset values */
                rsp.major = 0;
                rsp.minor = 0;
                rsp.patch = 0;

                Log_Write(LOG_LEVEL_ERROR,
                    "SQ[%d] HostCommandHandler:FW_VERSION_CMD:Request to SP failed:%d\r\n", sqw_idx,
                    status);
            }
        }
    }

#if TEST_FRAMEWORK
    /* For SP2MM command response, we need to provide the total size = header + payload */
    rsp.response_info.rsp_hdr.size = sizeof(struct device_ops_fw_version_rsp_t);
    status = SP_Iface_Push_Rsp_To_SP2MM_CQ(&rsp, sizeof(rsp));
#else
    rsp.response_info.rsp_hdr.size =
        sizeof(struct device_ops_fw_version_rsp_t) - sizeof(struct cmn_header_t);
    /* Push response to Host */
    status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));
#endif

    if (status == STATUS_SUCCESS)
    {
        /* Check for abort status for trace logging */
        if (rsp.status == DEV_OPS_API_FW_VERSION_RESPONSE_SUCCESS)
        {
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_SUCCEEDED)
        }
        else if (rsp.status == DEV_OPS_API_FW_VERSION_RESPONSE_HOST_ABORTED)
        {
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_ABORTED)
        }
        else
        {
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_FAILED)
        }

        Log_Write(LOG_LEVEL_DEBUG, "SQ[%d]:HostCmdHdlr:CQ_Push:FW_VERSION_CMD_RSP:tag_id=%x\r\n",
            sqw_idx, rsp.response_info.rsp_hdr.tag_id);
    }
    else
    {
        TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD, sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_FAILED)

        Log_Write(LOG_LEVEL_ERROR, "SQ[%d]:HostCmdHdlr:Tag_ID=%u:CQ_Push:Failed\r\n", sqw_idx,
            cmd->command_info.cmd_hdr.tag_id);
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
    }

#if !TEST_FRAMEWORK
    /* Decrement commands count being processed by given SQW */
    SQW_Decrement_Command_Count(sqw_idx);
#endif

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
static inline int8_t echo_cmd_handler(void *command_buffer, uint8_t sqw_idx, uint64_t start_cycles)
{
    const struct device_ops_echo_cmd_t *cmd = (struct device_ops_echo_cmd_t *)command_buffer;
    struct device_ops_echo_rsp_t rsp = { 0 };
    int8_t status = STATUS_SUCCESS;

    TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD, sqw_idx,
        cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_RECEIVED)

    Log_Write(LOG_LEVEL_DEBUG, "SQ[%d] HostCommandHandler:Processing:ECHO_CMD\r\n", sqw_idx);

    /* Construct and transmit response */
    rsp.response_info.rsp_hdr.tag_id = cmd->command_info.cmd_hdr.tag_id;
    rsp.response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_ECHO_RSP;
    rsp.status = DEV_OPS_API_ECHO_RESPONSE_SUCCESS;

    /* Get the SQW state to check for command abort */
    if (SQW_Get_State(sqw_idx) == SQW_STATE_ABORTED)
    {
        rsp.status = DEV_OPS_API_ECHO_RESPONSE_HOST_ABORTED;
    }
    else
    {
        TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD, sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_EXECUTING)
        rsp.device_cmd_start_ts = start_cycles;
    }

#if TEST_FRAMEWORK
    /* For SP2MM command response, we need to provide the total size = header + payload */
    rsp.response_info.rsp_hdr.size = sizeof(struct device_ops_echo_rsp_t);
    status = SP_Iface_Push_Rsp_To_SP2MM_CQ(&rsp, sizeof(rsp));
#else
    rsp.response_info.rsp_hdr.size =
        sizeof(struct device_ops_echo_rsp_t) - sizeof(struct cmn_header_t);
    status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));
#endif

    if (status == STATUS_SUCCESS)
    {
        /* Check for abort status for trace logging */
        if (rsp.status == DEV_OPS_API_ECHO_RESPONSE_HOST_ABORTED)
        {
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_ABORTED)
        }
        else
        {
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_SUCCEEDED)
        }

        Log_Write(LOG_LEVEL_DEBUG, "SQ[%d] HostCommandHandler:CQ_Push:ECHO_CMD_RSP:tag_id=%x\r\n",
            sqw_idx, rsp.response_info.rsp_hdr.tag_id);
    }
    else
    {
        TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD, sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_FAILED)

        Log_Write(LOG_LEVEL_ERROR, "SQ[%d]:HostCommandHandler:Tag_ID=%u:CQ_Push:Failed\r\n",
            sqw_idx, cmd->command_info.cmd_hdr.tag_id);
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
    }

#if !TEST_FRAMEWORK
    /* Decrement commands count being processed by given SQW */
    SQW_Decrement_Command_Count(sqw_idx);
#endif

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
static inline int8_t kernel_launch_cmd_handler(
    void *command_buffer, uint8_t sqw_idx, uint64_t start_cycles)
{
    struct device_ops_kernel_launch_cmd_t *cmd =
        (struct device_ops_kernel_launch_cmd_t *)command_buffer;
    struct device_ops_kernel_launch_rsp_t rsp;
    uint8_t kw_idx;
    exec_cycles_t cycles;
    int8_t status = GENERAL_ERROR;
    int8_t abort_status = STATUS_SUCCESS;

    TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD, sqw_idx,
        cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_RECEIVED)

    Log_Write(
        LOG_LEVEL_DEBUG, "SQ[%d] HostCommandHandler:Processing:KERNEL_LAUNCH_CMD\r\n", sqw_idx);

    /* Get the SQW state to check for command abort */
    if (SQW_Get_State(sqw_idx) == SQW_STATE_ABORTED)
    {
        abort_status = HOST_CMD_STATUS_ABORTED;
    }

    if (abort_status == STATUS_SUCCESS)
    {
        /* Blocking call to launch kernel */
        status = KW_Dispatch_Kernel_Launch_Cmd(cmd, sqw_idx, &kw_idx);
    }

    /* Compute Wait Cycles (cycles the command waits to launch on Compute Minions)
        Snapshot current cycle */
    cycles.cmd_start_cycles = start_cycles;
    cycles.wait_cycles = (PMC_GET_LATENCY(start_cycles) & 0xFFFFFFF);
    cycles.exec_start_cycles = ((uint32_t)PMC_Get_Current_Cycles() & 0xFFFFFFFF);

    if (status == STATUS_SUCCESS)
    {
        /* Notify kernel worker to aggregate kernel completion responses, and construct
        and transmit command response to host completion queue */
        KW_Notify(kw_idx, &cycles);

        Log_Write(LOG_LEVEL_DEBUG, "SQ[%d] HostCommandHandler:KW:%d:Notified\r\n", sqw_idx, kw_idx);
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
        rsp.response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP;
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
        else if ((status == KW_ABORTED_KERNEL_SLOT_SEARCH) ||
                 (status == KW_ABORTED_KERNEL_SHIRES_SEARCH) ||
                 (abort_status == HOST_CMD_STATUS_ABORTED))
        {
            rsp.status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED;
        }
        else
        {
            rsp.status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_ERROR;
        }

#if TEST_FRAMEWORK
        /* For SP2MM command response, we need to provide the total size = header + payload */
        rsp.response_info.rsp_hdr.size = sizeof(struct device_ops_kernel_launch_rsp_t);
        status = SP_Iface_Push_Rsp_To_SP2MM_CQ(&rsp, sizeof(rsp));
#else
        rsp.response_info.rsp_hdr.size =
            sizeof(struct device_ops_kernel_launch_rsp_t) - sizeof(struct cmn_header_t);
        status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));
#endif
        /* Check for abort status for trace logging.
        Since we are in failure path, we will ignore CQ push status for logging to trace. */
        if (rsp.status == DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED)
        {
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_ABORTED)
        }
        else
        {
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_FAILED)
        }

        if (status == STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_DEBUG,
                "SQ[%d] HostCommandHandler:Pushed:KERNEL_LAUNCH_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                sqw_idx, rsp.response_info.rsp_hdr.tag_id);
        }
        else
        {
            Log_Write(LOG_LEVEL_ERROR,
                "SQ[%d] HostCommandHandler:Tag_ID=%u:HostIface:Push:Failed\r\n", sqw_idx,
                cmd->command_info.cmd_hdr.tag_id);
            SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
        }

#if !TEST_FRAMEWORK
        /* Decrement commands count being processed by given SQW */
        SQW_Decrement_Command_Count(sqw_idx);
#endif
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
static inline int8_t kernel_abort_cmd_handler(void *command_buffer, uint8_t sqw_idx)
{
    struct device_ops_kernel_abort_cmd_t *cmd =
        (struct device_ops_kernel_abort_cmd_t *)command_buffer;
    struct device_ops_kernel_abort_rsp_t rsp;
    int8_t status = GENERAL_ERROR;
    int8_t abort_status = STATUS_SUCCESS;

    TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_CMD, sqw_idx,
        cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_RECEIVED)

    Log_Write(
        LOG_LEVEL_DEBUG, "SQ[%d] HostCommandHandler:Processing:KERNEL_ABORT_CMD\r\n", sqw_idx);

    /* Get the SQW state to check for command abort */
    if (SQW_Get_State(sqw_idx) == SQW_STATE_ABORTED)
    {
        abort_status = HOST_CMD_STATUS_ABORTED;
    }

    if (abort_status == STATUS_SUCCESS)
    {
        /* Dispatch kernel abort command */
        status = KW_Dispatch_Kernel_Abort_Cmd(cmd, sqw_idx);
    }

    if (status != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR,
            "HostCmdHdlr:KernelAbort:Failed:Status:%d:CmdParams:kernel_launch_tag_id:%x\r\n",
            status, cmd->kernel_launch_tag_id);

        /* Construct and transit command response */
        rsp.response_info.rsp_hdr.tag_id = cmd->command_info.cmd_hdr.tag_id;
        rsp.response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP;
        rsp.response_info.rsp_hdr.size =
            sizeof(struct device_ops_kernel_abort_rsp_t) - sizeof(struct cmn_header_t);

        /* Populate the error type response */
        if (abort_status == HOST_CMD_STATUS_ABORTED)
        {
            rsp.status = DEV_OPS_API_KERNEL_ABORT_RESPONSE_HOST_ABORTED;
        }
        else if ((status == KW_ERROR_KERNEL_SLOT_NOT_FOUND) ||
                 (status == KW_ERROR_KERNEL_SLOT_NOT_USED))
        {
            rsp.status = DEV_OPS_API_KERNEL_ABORT_RESPONSE_INVALID_TAG_ID;
        }
        else
        {
            rsp.status = DEV_OPS_API_KERNEL_ABORT_RESPONSE_ERROR;
        }

        status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

        /* Check for abort status for trace logging.
        Since we are in failure path, we will ignore CQ push status for logging to trace. */
        if (rsp.status == DEV_OPS_API_KERNEL_ABORT_RESPONSE_HOST_ABORTED)
        {
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_ABORTED)
        }
        else
        {
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_FAILED)
        }

        if (status == STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_DEBUG,
                "HostCommandHandler:Pushed:KERNEL_ABORT_CMD_RSP:tag_id=%x->Host_CQ\r\n",
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

    /* If flags are set to extract both MM and CM Trace buffers. */
    if ((cmd->command_info.cmd_hdr.flags & CMD_FLAGS_MMFW_TRACEBUF) &&
        (cmd->command_info.cmd_hdr.flags & CMD_FLAGS_CMFW_TRACEBUF))
    {
        cm_shire_mask = Trace_Get_CM_Shire_Mask();

        if ((cmd->list[TRACE_NODE_INDEX].size <= (MM_TRACE_BUFFER_SIZE + CM_TRACE_BUFFER_SIZE)) &&
            (cm_shire_mask & CW_Get_Booted_Shires()) == cm_shire_mask)
        {
            /* Disable and Evict MM Trace.*/
            Trace_Set_Enable_MM(TRACE_DISABLE);

            mm_to_cm_message_trace_buffer_evict_t cm_msg = {
                .header.id = MM_TO_CM_MESSAGE_ID_TRACE_BUFFER_EVICT,
                .thread_mask = Trace_Get_CM_Thread_Mask()
            };

            /* Send command to CM RT to disable Trace and evict Trace buffer. */
            status = CM_Iface_Multicast_Send(cm_shire_mask, (cm_iface_message_t *)&cm_msg);

            cmd->list[TRACE_NODE_INDEX].src_device_phy_addr = MM_TRACE_BUFFER_BASE;
        }
        else
        {
            status = DMA_ERROR_OUT_OF_BOUNDS;
        }
    }
    /* Check if flag is set to extract MM Trace buffer. */
    else if (cmd->command_info.cmd_hdr.flags & CMD_FLAGS_MMFW_TRACEBUF)
    {
        if (cmd->list[TRACE_NODE_INDEX].size <= MM_TRACE_BUFFER_SIZE)
        {
            /* Disable and Evict MM Trace.*/
            Trace_Set_Enable_MM(TRACE_DISABLE);

            cmd->list[TRACE_NODE_INDEX].src_device_phy_addr = MM_TRACE_BUFFER_BASE;
        }
        else
        {
            status = DMA_ERROR_OUT_OF_BOUNDS;
        }
    }
    /* Check if flag is set to extract CM Trace buffer. */
    else if (cmd->command_info.cmd_hdr.flags & CMD_FLAGS_CMFW_TRACEBUF)
    {
        cm_shire_mask = Trace_Get_CM_Shire_Mask();

        if ((cmd->list[TRACE_NODE_INDEX].size <= CM_TRACE_BUFFER_SIZE) &&
            ((cm_shire_mask & CW_Get_Booted_Shires()) == cm_shire_mask))
        {
            mm_to_cm_message_trace_buffer_evict_t cm_msg = {
                .header.id = MM_TO_CM_MESSAGE_ID_TRACE_BUFFER_EVICT,
                .thread_mask = Trace_Get_CM_Thread_Mask()
            };

            /* Send command to CM RT to disable Trace and evict Trace buffer. */
            status = CM_Iface_Multicast_Send(cm_shire_mask, (cm_iface_message_t *)&cm_msg);

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
static inline int8_t dma_readlist_cmd_handler(
    void *command_buffer, uint8_t sqw_idx, uint64_t start_cycles)
{
    struct device_ops_dma_readlist_cmd_t *cmd =
        (struct device_ops_dma_readlist_cmd_t *)command_buffer;
    struct device_ops_dma_readlist_rsp_t rsp;
    dma_flags_e dma_flag;
    dma_write_chan_id_e chan = DMA_CHAN_ID_WRITE_INVALID;
    int8_t status = GENERAL_ERROR;
    int8_t abort_status = STATUS_SUCCESS;
    uint64_t total_dma_size = 0;
    uint8_t dma_xfer_count = 0;
    uint8_t loop_cnt;
    exec_cycles_t cycles;

    TRACE_LOG_CMD_STATUS(cmd->command_info.cmd_hdr.msg_id, sqw_idx,
        cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_RECEIVED)

    /* Design Notes: Note a DMA write command from host will
    trigger the implementation to configure a DMA read channel
    on device to move data from host to device, similarly a read
    command from host will trigger the implementation to configure
    a DMA write channel on device to move data from device to host */
    Log_Write(LOG_LEVEL_DEBUG, "SQ[%d] HostCommandHandler:Processing:DATA_READ_CMD\r\n", sqw_idx);

    /* Get the SQW state to check for command abort */
    if (SQW_Get_State(sqw_idx) == SQW_STATE_ABORTED)
    {
        abort_status = HOST_CMD_STATUS_ABORTED;
    }

    if (abort_status == STATUS_SUCCESS)
    {
        /* Check if no special flag is set. */
        if ((cmd->command_info.cmd_hdr.flags & CMD_FLAGS_MMFW_TRACEBUF) ||
            (cmd->command_info.cmd_hdr.flags & CMD_FLAGS_CMFW_TRACEBUF))
        {
            dma_xfer_count = 1;
            dma_flag = DMA_SOC_NO_BOUNDS_CHECK;
            status = dma_readlist_cmd_process_trace_flags(cmd);
        }
        else
        {
            dma_flag = DMA_NORMAL;
            /* Get number of transfer commands in the list, based on message payload length. */
            dma_xfer_count = (uint8_t)((cmd->command_info.cmd_hdr.size - DEVICE_CMD_HEADER_SIZE) /
                                       sizeof(struct dma_read_node));

            /* Ensure the size of Xfer is bigger than zero */
            status = ((dma_xfer_count > 0) && (dma_xfer_count <= DEVICE_OPS_DMA_LIST_NODES_MAX)) ?
                         STATUS_SUCCESS :
                         DMA_ERROR_INVALID_XFER_COUNT;
        }
    }

    if (status == STATUS_SUCCESS)
    {
        /* Obtain the next available DMA write channel */
        status = DMAW_Write_Find_Idle_Chan_And_Reserve(&chan, sqw_idx);
    }

    if (status == STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_DEBUG, "SQ[%d] DMA_READ:channel_used:%d, dma xfer count=%d\r\n",
            sqw_idx, chan, dma_xfer_count);

        for (loop_cnt = 0; loop_cnt < dma_xfer_count; ++loop_cnt)
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

        /* Initiate DMA write transfer */
        status = DMAW_Write_Trigger_Transfer(chan, cmd, dma_xfer_count, sqw_idx, &cycles, dma_flag);
    }

    if (status != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "SQ[%d]:TID:%u:HostCmdHdlr:DMARead:Fail:%d\r\n", sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, status);

        for (loop_cnt = 0; loop_cnt < dma_xfer_count; ++loop_cnt)
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
        /* TODO: SW-9022: Add dma_readlist_cmd handling
        rsp.response_info.rsp_hdr.msg_id =
            DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP */
        rsp.response_info.rsp_hdr.size = sizeof(rsp) - sizeof(struct cmn_header_t);
        /* Compute Wait Cycles (cycles the command was sitting
        in SQ prior to launch) Snapshot current cycle */
        rsp.device_cmd_start_ts = start_cycles;
        rsp.device_cmd_wait_dur = (PMC_GET_LATENCY(start_cycles) & 0xFFFFFFF);
        rsp.device_cmd_execute_dur = 0U;

        /* Populate the error type response */
        DMA_TO_DEVICEAPI_STATUS(status, abort_status, rsp.status)

        Log_Write(LOG_LEVEL_DEBUG,
            "HostCommandHandler:Pushing:DATA_READ_CMD_RSP:tag_id=%x->Host_CQ\r\n",
            rsp.response_info.rsp_hdr.tag_id);

        status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

        /* Check for abort status for trace logging.
        Since we are in failure path, we will ignore CQ push status for logging to trace. */
        if (rsp.status == DEV_OPS_API_DMA_RESPONSE_HOST_ABORTED)
        {
            TRACE_LOG_CMD_STATUS(cmd->command_info.cmd_hdr.msg_id, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_ABORTED)
        }
        else
        {
            TRACE_LOG_CMD_STATUS(cmd->command_info.cmd_hdr.msg_id, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_FAILED)
        }

        if (status != STATUS_SUCCESS)
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
static inline int8_t dma_writelist_cmd_handler(
    void *command_buffer, uint8_t sqw_idx, uint64_t start_cycles)
{
    const struct device_ops_dma_writelist_cmd_t *cmd =
        (struct device_ops_dma_writelist_cmd_t *)command_buffer;
    struct device_ops_dma_writelist_rsp_t rsp;
    dma_read_chan_id_e chan = DMA_CHAN_ID_READ_INVALID;
    uint64_t total_dma_size = 0;
    uint8_t dma_xfer_count = 0;
    uint8_t loop_cnt;
    int8_t status = GENERAL_ERROR;
    int8_t abort_status = STATUS_SUCCESS;
    exec_cycles_t cycles;

    TRACE_LOG_CMD_STATUS(cmd->command_info.cmd_hdr.msg_id, sqw_idx,
        cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_RECEIVED)

    /* Design Notes: Note a DMA write command from host will trigger
    the implementation to configure a DMA read channel on device to move
    data from host to device, similarly a read command from host will
    trigger the implementation to configure a DMA write channel on device
    to move data from device to host */
    Log_Write(LOG_LEVEL_DEBUG, "SQ[%d] HostCommandHandler:Processing:DATA_WRITE_CMD\r\n", sqw_idx);

    /* Get the SQW state to check for command abort */
    if (SQW_Get_State(sqw_idx) == SQW_STATE_ABORTED)
    {
        abort_status = HOST_CMD_STATUS_ABORTED;
    }

    if (abort_status == STATUS_SUCCESS)
    {
        /* Get number of transfer commands in the list, based on message payload length. */
        dma_xfer_count = (uint8_t)((cmd->command_info.cmd_hdr.size - DEVICE_CMD_HEADER_SIZE) /
                                   sizeof(struct dma_write_node));

        /* Obtain the next available DMA read channel */
        status = DMAW_Read_Find_Idle_Chan_And_Reserve(&chan, sqw_idx);

        /* Ensure the size of Xfer is bigger than zero */
        status = ((dma_xfer_count > 0) && (dma_xfer_count <= DEVICE_OPS_DMA_LIST_NODES_MAX)) ?
                     status :
                     DMA_ERROR_INVALID_XFER_COUNT;
    }

    if (status == STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_DEBUG, "SQ[%d] DMA_WRITE:channel_used:%d, dma xfer count=%d \r\n",
            sqw_idx, chan, dma_xfer_count);

        for (loop_cnt = 0; loop_cnt < dma_xfer_count; ++loop_cnt)
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

        /* Initiate DMA read transfer */
        status = DMAW_Read_Trigger_Transfer(chan, cmd, dma_xfer_count, sqw_idx, &cycles);
    }

    if (status != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "HostCmdHdlr:DMAWrite:TID:%u:Fail:%d\r\n",
            cmd->command_info.cmd_hdr.tag_id, status);

        for (loop_cnt = 0; loop_cnt < dma_xfer_count; ++loop_cnt)
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
        /* TODO: SW-9022: Add dma_writelist_cmd handling
        rsp.response_info.rsp_hdr.msg_id =
            DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP */
        rsp.response_info.rsp_hdr.size = sizeof(rsp) - sizeof(struct cmn_header_t);
        /* Compute Wait Cycles (cycles the command was sitting
        in SQ prior to launch) Snapshot current cycle */
        rsp.device_cmd_start_ts = start_cycles;
        rsp.device_cmd_wait_dur = (PMC_GET_LATENCY(start_cycles) & 0xFFFFFFF);
        rsp.device_cmd_execute_dur = 0U;

        /* Populate the error type response */
        DMA_TO_DEVICEAPI_STATUS(status, abort_status, rsp.status)

        status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

        /* Check for abort status for trace logging.
        Since we are in failure path, we will ignore CQ push status for logging to trace. */
        if (rsp.status == DEV_OPS_API_DMA_RESPONSE_HOST_ABORTED)
        {
            TRACE_LOG_CMD_STATUS(cmd->command_info.cmd_hdr.msg_id, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_ABORTED)
        }
        else
        {
            TRACE_LOG_CMD_STATUS(cmd->command_info.cmd_hdr.msg_id, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_FAILED)
        }

        if (status == STATUS_SUCCESS)
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
static inline int8_t trace_rt_control_cmd_handler(void *command_buffer, uint8_t sqw_idx)
{
    const struct device_ops_trace_rt_control_cmd_t *cmd =
        (struct device_ops_trace_rt_control_cmd_t *)command_buffer;
    struct device_ops_trace_rt_control_rsp_t rsp;
    int8_t status = STATUS_SUCCESS;

    TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_CMD, sqw_idx,
        cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_RECEIVED)

    Log_Write(LOG_LEVEL_DEBUG, "HostCommandHandler:Processing:TRACE_RT_CONTROL_CMD\r\n");

    /* Get the SQW state to check for command abort */
    if (SQW_Get_State(sqw_idx) == SQW_STATE_ABORTED)
    {
        status = HOST_CMD_STATUS_ABORTED;
    }
    else
    {
        TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_CMD, sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_EXECUTING)
    }

    /* Check if RT Component is MM Trace. */
    if ((status == STATUS_SUCCESS) && (cmd->rt_type & TRACE_RT_TYPE_MM))
    {
        Log_Write(LOG_LEVEL_DEBUG, "HostCommandHandler:TRACE_RT_CONTROL_CMD:MM RT control\r\n");
        Trace_RT_Control_MM(cmd->control);
    }

    /* Check if RT Component is CM Trace. */
    if ((status == STATUS_SUCCESS) && (cmd->rt_type & TRACE_RT_TYPE_CM))
    {
        Log_Write(LOG_LEVEL_DEBUG, "HostCommandHandler:TRACE_RT_CONTROL_CMD:CM RT control\r\n");

        mm_to_cm_message_trace_rt_control_t cm_msg;
        cm_msg.header.id = MM_TO_CM_MESSAGE_ID_TRACE_UPDATE_CONTROL;
        cm_msg.cm_control = cmd->control;
        cm_msg.thread_mask = Trace_Get_CM_Thread_Mask();

        uint64_t cm_shire_mask = Trace_Get_CM_Shire_Mask();

        if ((cm_shire_mask & CW_Get_Booted_Shires()) != cm_shire_mask)
        {
            status = INVALID_CM_SHIRE_MASK;
        }
        else
        {
            /* Send command to CM RT to disable Trace and evict Trace buffer. */
            status = CM_Iface_Multicast_Send(cm_shire_mask, (cm_iface_message_t *)&cm_msg);
        }

        if (status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR,
                "TRACE_RT_CONTROL:CM:Tag_ID=%u:Failed to Enable/Disable Trace, and to redirect logs.\r\n",
                cmd->command_info.cmd_hdr.tag_id);
        }
    }

    /* Construct and transmit command response. */
    rsp.response_info.rsp_hdr.tag_id = cmd->command_info.cmd_hdr.tag_id;
    rsp.response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_RSP;
    rsp.response_info.rsp_hdr.size = sizeof(rsp) - sizeof(struct cmn_header_t);

    /* Populate the response status */
    if (status == HOST_CMD_STATUS_ABORTED)
    {
        rsp.status = DEV_OPS_TRACE_RT_CONTROL_RESPONSE_HOST_ABORTED;
    }
    else if (!((cmd->rt_type & TRACE_RT_TYPE_MM) || (cmd->rt_type & TRACE_RT_TYPE_CM)))
    {
        rsp.status = DEV_OPS_TRACE_RT_CONTROL_RESPONSE_BAD_RT_TYPE;
    }
    else if (status == STATUS_SUCCESS)
    {
        rsp.status = DEV_OPS_TRACE_RT_CONTROL_RESPONSE_SUCCESS;
    }
    else if (status == INVALID_CM_SHIRE_MASK)
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

#if TEST_FRAMEWORK
    /* For SP2MM command response, we need to provide the total size = header + payload */
    rsp.response_info.rsp_hdr.size = sizeof(struct device_ops_trace_rt_control_rsp_t);
    status = SP_Iface_Push_Rsp_To_SP2MM_CQ(&rsp, sizeof(rsp));
#else
    status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));
#endif

    if (status != STATUS_SUCCESS)
    {
        TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_CMD, sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_FAILED)
        Log_Write(LOG_LEVEL_ERROR, "HostCommandHandler:Tag_ID=%u:HostIface:Push:Failed\r\n",
            cmd->command_info.cmd_hdr.tag_id);
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
    }
    else
    {
        /* Check for abort status for trace logging. */
        if (rsp.status == DEV_OPS_TRACE_RT_CONTROL_RESPONSE_HOST_ABORTED)
        {
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_ABORTED)
        }
        else if (rsp.status == DEV_OPS_TRACE_RT_CONTROL_RESPONSE_SUCCESS)
        {
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_SUCCEEDED)
        }
        else
        {
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_FAILED)
        }
    }

#if !TEST_FRAMEWORK
    /* Decrement commands count being processed by given SQW */
    SQW_Decrement_Command_Count(sqw_idx);
#endif

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       trace_rt_config_cmd_handler
*
*   DESCRIPTION
*
*       Process host Trace runtime config command, and transmit response.
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
static inline int8_t trace_rt_config_cmd_handler(void *command_buffer, uint8_t sqw_idx)
{
    const struct device_ops_trace_rt_config_cmd_t *cmd =
        (struct device_ops_trace_rt_config_cmd_t *)command_buffer;
    struct device_ops_trace_rt_config_rsp_t rsp;
    int8_t status = STATUS_SUCCESS;

    TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_CMD, sqw_idx,
        cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_RECEIVED)

    Log_Write(LOG_LEVEL_DEBUG, "HostCmdHdlr:TID:%u:TRACE_CONFIG:Shire:%lx:Thread:%lx\r\n",
        cmd->command_info.cmd_hdr.tag_id, cmd->shire_mask, cmd->thread_mask);

    /* Get the SQW state to check for command abort */
    if (SQW_Get_State(sqw_idx) == SQW_STATE_ABORTED)
    {
        status = HOST_CMD_STATUS_ABORTED;
    }
    else
    {
        TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_CMD, sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_EXECUTING)

        /* Check if MM Trace needs to configured. */
        if (TRACE_CONFIG_CHECK_MM_HART(cmd->shire_mask, cmd->thread_mask))
        {
            struct trace_init_info_t mm_trace_init = { .shire_mask = MM_SHIRE_MASK,
                .thread_mask = MM_HART_MASK,
                .filter_mask = cmd->filter_mask,
                .event_mask = cmd->event_mask,
                .threshold = MM_TRACE_BUFFER_SIZE };

            /* Configure MM Trace. */
            Trace_Init_MM(&mm_trace_init);
            Trace_String(
                TRACE_EVENT_STRING_CRITICAL, Trace_Get_MM_CB(), "MM:TRACE_RT_CONFIG:Done!!");
        }

        /* Check if CM Trace needs to configured. */
        if (TRACE_CONFIG_CHECK_CM_HART(cmd->shire_mask, cmd->thread_mask))
        {
            Log_Write(LOG_LEVEL_DEBUG, "HostCmdHdlr:TID:%u:TRACE_CONFIG: Configure CM.\r\n",
                cmd->command_info.cmd_hdr.tag_id);

            if ((cmd->shire_mask & CW_Get_Booted_Shires()) == cmd->shire_mask)
            {
                mm_to_cm_message_trace_rt_config_t cm_msg;
                cm_msg.header.id = MM_TO_CM_MESSAGE_ID_TRACE_CONFIGURE;
                cm_msg.shire_mask = cmd->shire_mask;
                cm_msg.thread_mask = cmd->thread_mask;
                cm_msg.filter_mask = cmd->filter_mask;
                cm_msg.event_mask = cmd->event_mask;
                cm_msg.threshold = cmd->threshold;

                status = Trace_Configure_CM_RT(&cm_msg);
            }
            else
            {
                status = INVALID_CM_SHIRE_MASK;
            }
        }
    }

    /* Construct and transmit command response. */
    rsp.response_info.rsp_hdr.tag_id = cmd->command_info.cmd_hdr.tag_id;
    rsp.response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_RSP;
    rsp.response_info.rsp_hdr.size = sizeof(rsp) - sizeof(struct cmn_header_t);

    /* Populate the response status */
    if (status == HOST_CMD_STATUS_ABORTED)
    {
        rsp.status = DEV_OPS_TRACE_RT_CONFIG_RESPONSE_HOST_ABORTED;
    }
    if ((cmd->shire_mask & MM_SHIRE_MASK) && (!(cmd->thread_mask & MM_HART_MASK)))
    {
        rsp.status = DEV_OPS_TRACE_RT_CONFIG_RESPONSE_BAD_THREAD_MASK;
    }
    else if ((status == INVALID_CM_SHIRE_MASK) ||
             ((!(cmd->shire_mask & MM_SHIRE_MASK)) && (!(cmd->shire_mask & CM_SHIRE_MASK))))
    {
        rsp.status = DEV_OPS_TRACE_RT_CONFIG_RESPONSE_BAD_SHIRE_MASK;
    }
    else if (status == STATUS_SUCCESS)
    {
        rsp.status = DEV_OPS_TRACE_RT_CONFIG_RESPONSE_SUCCESS;
    }
    else
    {
        rsp.status = DEV_OPS_TRACE_RT_CONFIG_RESPONSE_RT_CONFIG_ERROR;
    }

    Log_Write(LOG_LEVEL_DEBUG, "HostCmdHdlr:Pushing:TRACE_RT_CONFIG_RSP:tag_id=%x:Host_CQ\r\n",
        rsp.response_info.rsp_hdr.tag_id);

#if TEST_FRAMEWORK
    /* For SP2MM command response, we need to provide the total size = header + payload */
    rsp.response_info.rsp_hdr.size = sizeof(struct device_ops_trace_rt_config_rsp_t);
    status = SP_Iface_Push_Rsp_To_SP2MM_CQ(&rsp, sizeof(rsp));
#else
    status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));
#endif

    if (status != STATUS_SUCCESS)
    {
        TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_CMD, sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_FAILED)
        Log_Write(LOG_LEVEL_ERROR, "HostCmdHdlr:Tag_ID=%u:HostIface:Push:Failed\r\n",
            cmd->command_info.cmd_hdr.tag_id);
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
    }
    /* Check for abort status for trace logging. */
    else if (rsp.status == DEV_OPS_TRACE_RT_CONFIG_RESPONSE_HOST_ABORTED)
    {
        TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_CMD, sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_ABORTED)
    }
    else if (rsp.status == DEV_OPS_TRACE_RT_CONFIG_RESPONSE_SUCCESS)
    {
        TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_CMD, sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_SUCCEEDED)
    }
    else
    {
        TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_CMD, sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_FAILED)
    }

#if !TEST_FRAMEWORK
    /* Decrement commands count being processed by given SQW */
    SQW_Decrement_Command_Count(sqw_idx);
#endif

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       device_unsupported_cmd_event_handler
*
*   DESCRIPTION
*
*       Constructs unsupported command error event for host and pushes to
*       CQ.
*
*   INPUTS
*
*       command_buffer   Buffer containing command info
*       sqw_idx          Submission queue index (normal or HP)
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
static inline void device_unsupported_cmd_event_handler(void *command_buffer, uint8_t sqw_idx)
{
    const struct cmn_header_t *cmd_header = (struct cmn_header_t *)command_buffer;
    struct device_ops_device_fw_error_t event;
    int8_t status;

    /* Fill the event */
    event.event_info.event_hdr.tag_id = cmd_header->tag_id;
    event.event_info.event_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_ERROR;
    event.error_type = DEV_OPS_API_ERROR_TYPE_UNSUPPORTED_COMMAND;

    /* Push the event to CQ */
    status = Host_Iface_CQ_Push_Cmd(0, &event, sizeof(event));

    if (status == STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_DEBUG,
            "SQ[%d] fw_error_event:Pushed:Async error event:tag_id=%x->Host_CQ\r\n", sqw_idx,
            event.event_info.event_hdr.tag_id);
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "SQ[%d] fw_error_event:Tag_ID=%u:HostIface:Push:Failed\r\n",
            sqw_idx, event.event_info.event_hdr.tag_id);

        SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
    }
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
int8_t Host_Command_Handler(void *command_buffer, uint8_t sqw_idx, uint64_t start_cycles)
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
        case DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_CMD:
            status = trace_rt_config_cmd_handler(command_buffer, sqw_idx);
            break;
        default:
            Log_Write(
                LOG_LEVEL_ERROR, "HostCmdHdlr:Tag_ID=%u:UnsupportedCmd\r\n", hdr->cmd_hdr.tag_id);

            /* TODO: Send unsupported command error event to host
            Requires updates to PCIe kernel driver/userspace test */

            /* Decrement commands count being processed by given SQW */
            SQW_Decrement_Command_Count(sqw_idx);

            status = GENERAL_ERROR;
            break;
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       Host_HP_Command_Handler
*
*   DESCRIPTION
*
*       Process host high priority command, and transmit response as needed
*
*   INPUTS
*
*       command_buffer   Buffer containing command to process
*       sqw_hp_idx       HP Submission queue index
*
*   OUTPUTS
*
*       int8_t           Successful status or error code.
*
***********************************************************************/
int8_t Host_HP_Command_Handler(void *command_buffer, uint8_t sqw_hp_idx)
{
    int8_t status = STATUS_SUCCESS;
    const struct cmd_header_t *hdr = command_buffer;

    switch (hdr->cmd_hdr.msg_id)
    {
        case DEV_OPS_API_MID_DEVICE_OPS_ABORT_CMD:
            status = abort_cmd_handler(command_buffer, sqw_hp_idx);
            break;
        default:
            Log_Write(LOG_LEVEL_ERROR, "HostCmdHdlr_HP:Tag_ID=%u:UnsupportedCmd\r\n",
                hdr->cmd_hdr.tag_id);

            /* Send unsupported command error event to host */
            device_unsupported_cmd_event_handler(command_buffer, sqw_hp_idx);

            /* Decrement commands count being processed by given HP SQW */
            SQW_HP_Decrement_Command_Count(sqw_hp_idx);

            status = GENERAL_ERROR;
            break;
    }

    return status;
}
