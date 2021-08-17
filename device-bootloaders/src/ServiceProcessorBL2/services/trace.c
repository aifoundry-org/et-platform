/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file trace.c
    \brief A C module that implements the Trace services

    Public interfaces:
        Trace_Init_SP
        Trace_Get_SP_CB

*/
/***********************************************************************/

#include "log.h"
#include <string.h>
#include "config/mgmt_build_config.h"

#define ET_TRACE_ENCODER_IMPL
#include "trace_sp_primitives.h"
#include "trace.h"

/*
 * Service Processor Trace control block.
 */
struct trace_control_block_t SP_Trace_CB;

static void Trace_Run_Control(trace_enable_e state);
static void Trace_Configure(uint32_t event_mask, uint32_t filter_mask);

void Trace_Process_Control_Cmd(void *buffer)
{
    struct device_mgmt_trace_run_control_cmd_t *dm_cmd =
        (struct device_mgmt_trace_run_control_cmd_t *)buffer;

    /* Check flag to Enable/Disable Trace. */
    if (dm_cmd->control & SP_TRACE_ENABLE)
    {
        Trace_Run_Control(TRACE_ENABLE);
        Log_Write(LOG_LEVEL_INFO,
                            "TRACE_RT_CONTROL:SP:Trace Enabled.\r\n");
    }
    else
    {
        Trace_Run_Control(TRACE_DISABLE);
        Log_Write(LOG_LEVEL_INFO,
                            "TRACE_RT_CONTROL:SP:Trace Disabled.\r\n");
    }

    if (dm_cmd->control & SP_TRACE_UART_ENABLE)
    {
        Log_Set_Interface(LOG_DUMP_TO_UART);
        Log_Write(LOG_LEVEL_INFO,
                "TRACE_RT_CONTROL:SP:Logs redirected to UART.\r\n");
    }
    else
    {
        Log_Set_Interface(LOG_DUMP_TO_TRACE);
        Log_Write(LOG_LEVEL_INFO,
                "TRACE_RT_CONTROL:SP:Logs redirected to Trace buffer\r\n");

    }
}

static void send_trace_control_response(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time)
{
    struct device_mgmt_default_rsp_t dm_rsp;

    FILL_RSP_HEADER(dm_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time,
                     DM_STATUS_SUCCESS);

    dm_rsp.payload = DM_STATUS_SUCCESS;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp,
                                        sizeof(struct device_mgmt_default_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "send_trace_control_response: Cqueue push error!\n");
    }
}

void Trace_Process_Config_Cmd(void *buffer)
{
    struct device_mgmt_trace_config_cmd_t *dm_cmd =
        (struct device_mgmt_trace_config_cmd_t *)buffer;

    Trace_Configure(dm_cmd->event_mask, dm_cmd->filter_mask);
          Log_Write(LOG_LEVEL_INFO,
                            "TRACE_CONFIG:SP:Trace Event/Filter Mask set.\r\n");
}

static void send_trace_config_response(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time)
{
    struct device_mgmt_default_rsp_t dm_rsp;

    FILL_RSP_HEADER(dm_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time,
                     DM_STATUS_SUCCESS);

    dm_rsp.payload = DM_STATUS_SUCCESS;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp,
                                        sizeof(struct device_mgmt_trace_config_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "send_trace_config_response: Cqueue push error!\n");
    }
}
/************************************************************************
*
*   FUNCTION
*
*       Trace_Init_SP
*
*   DESCRIPTION
*
*       This function initializes Trace for Service Processor
*
*   INPUTS
*
*       trace_init_info_t    Trace init info for Service Processor .
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Init_SP(const struct trace_init_info_t *sp_init_info)
{
    struct trace_init_info_t sp_init_info_l;

    /* If init information is NULL then do default initialization. */
    if (sp_init_info == NULL)
    {
        /* Populate default Trace configurations for Service Processor. */
        sp_init_info_l.buffer        = SP_TRACE_BUFFER_BASE;
        sp_init_info_l.buffer_size   = SP_TRACE_BUFFER_SIZE;
        sp_init_info_l.event_mask    = TRACE_EVENT_STRING;
        sp_init_info_l.filter_mask   = TRACE_EVENT_STRING_WARNING;
        sp_init_info_l.threshold     = SP_TRACE_BUFFER_SIZE;
    }
    else
    {
        memcpy(&sp_init_info_l, sp_init_info, sizeof(struct trace_init_info_t));
    }

    /* Common buffer for all SP HART. */
    SP_Trace_CB.size_per_hart = SP_TRACE_BUFFER_SIZE;
    SP_Trace_CB.base_per_hart = SP_TRACE_BUFFER_BASE;

    /* Initialize Trace for each all Harts in Service Processor. */
    Trace_Init(&sp_init_info_l, &SP_Trace_CB, TRACE_STD_HEADER);
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Get_SP_CB
*
*   DESCRIPTION
*
*       This function returns the common Trace control block (CB) for all
*       SP Harts.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       trace_control_block_t Pointer to the Trace control block.
*
***********************************************************************/
struct trace_control_block_t* Trace_Get_SP_CB(void)
{
    return &SP_Trace_CB;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Get_SP_Buffer
*
*   DESCRIPTION
*
*       This function returns the common Trace control block buffer offset per
*       SP Hart.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       offset value per hart.
*
***********************************************************************/
uint32_t Trace_Get_SP_Buffer(void)
{
    return SP_Trace_CB.offset_per_hart;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Run_Control
*
*   DESCRIPTION
*
*       This function enable/disable Trace for Service Processor.
*
*   INPUTS
*
*       trace_enable_e    Enable/Disable Trace.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void Trace_Run_Control(trace_enable_e state)
{
    SP_Trace_CB.enable = state;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Configure
*
*   DESCRIPTION
*
*       This function sets event/filter mask of Trace for Service Processor.
*
*   INPUTS
*
*       uint32_t                     Trace event Mask.
*       uint32_t                     Trace filter Mask.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void Trace_Configure(uint32_t event_mask, uint32_t filter_mask)
{
       SP_Trace_CB.event_mask = event_mask;
       SP_Trace_CB.filter_mask = filter_mask;
}
/************************************************************************
*
*   FUNCTION
*
*       Trace_Process_CMD
*
*   DESCRIPTION
*
*       This function process host commands for Trace.
*
*   INPUTS
*
*       tag_id      Unique enum representing specific command
*       msg_id      Unique enum representing specific command
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Process_CMD(tag_id_t tag_id, msg_id_t msg_id, void *buffer)
{
    uint64_t req_start_time = timer_get_ticks_count();

    switch (msg_id)
    {
        case DM_CMD_SET_DM_TRACE_RUN_CONTROL:

            /* process trace control command */
            Trace_Process_Control_Cmd(buffer);

            /* send response for trace control command */
            send_trace_control_response(tag_id, msg_id, req_start_time);
            break;

        case DM_CMD_SET_DM_TRACE_CONFIG:

            /* process trace config command */
            Trace_Process_Config_Cmd(buffer);

            /* send response for trace config command */
            send_trace_config_response(tag_id, msg_id, req_start_time);
            break;
        default:
            break;
    }
}
