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
    \brief A C module that implements the SP Trace services. This file
    also provides the required primitives for trace Encoder.

    Public interfaces:
        Trace_Init_SP
        Trace_Get_SP_CB
        Trace_Get_SP_Buffer
        Trace_Run_Control
        Trace_Configure
        Trace_Process_CMD

*/
/***********************************************************************/

#include "config/mgmt_build_config.h"
#include "bl2_scratch_buffer.h"
#include "bl2_timer.h"
#include "FreeRTOS.h"
#include "portmacro.h"
#include "semphr.h"
#include "task.h"
#include <string.h>

/* Encoder function prototypes */
static inline void et_trace_buffer_lock_acquire(void);
static inline void et_trace_buffer_lock_release(void);

/* Master Minion trace buffer locks */
#define ET_TRACE_BUFFER_LOCK_ACQUIRE      et_trace_buffer_lock_acquire();
#define ET_TRACE_BUFFER_LOCK_RELEASE      et_trace_buffer_lock_release();

#define ET_TRACE_GET_TIMESTAMP()          timer_get_ticks_count()

/* Define for Encoder */
#define ET_TRACE_ENCODER_IMPL
#include "trace.h"

/*
 * Service Processor Trace control block.
 */
struct trace_control_block_t SP_Trace_CB;

/* Trace buffer lock */
static SemaphoreHandle_t Trace_Mutex_Handle = NULL;
static StaticSemaphore_t Trace_Mutex_Buffer;

static void Trace_Run_Control(trace_enable_e state);
static void Trace_Configure(uint32_t event_mask, uint32_t filter_mask);

/* Trace buffer locking routines */
static inline void et_trace_buffer_lock_acquire(void)
{
    /* Acquire the Mutex */
    if (Trace_Mutex_Handle && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED))
    {
        xSemaphoreTake(Trace_Mutex_Handle, portMAX_DELAY);
    }
}

static inline void et_trace_buffer_lock_release(void)
{
    /* Release the Mutex */
    if (Trace_Mutex_Handle && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED))
    {
        xSemaphoreGive(Trace_Mutex_Handle);
    }
}

void Trace_Process_Control_Cmd(void *buffer)
{
    struct device_mgmt_trace_run_control_cmd_t *dm_cmd =
        (struct device_mgmt_trace_run_control_cmd_t *)buffer;

    struct trace_buffer_std_header_t *trace_header =
        (struct trace_buffer_std_header_t *)SP_Trace_CB.base_per_hart;

    /* Check flag to reset trace buffer. */
    if (dm_cmd->control & TRACE_CONTROL_RESET_TRACEBUF)
    {
        /* Reset the trace buffer */
        trace_header->data_size = sizeof(struct trace_buffer_std_header_t);
        SP_Trace_CB.offset_per_hart = sizeof(struct trace_buffer_std_header_t);
    }

    /* Check flag to Enable/Disable Trace. */
    if (dm_cmd->control & TRACE_CONTROL_TRACE_ENABLE)
    {
        Trace_Run_Control(TRACE_ENABLE);
        Log_Write(LOG_LEVEL_INFO,
                            "TRACE_RT_CONTROL:SP:Trace Enabled.\r\n");
    }
    else
    {
        trace_header->data_size = SP_Trace_CB.offset_per_hart;
        Trace_Run_Control(TRACE_DISABLE);
        //NOSONAR TODO: https://esperantotech.atlassian.net/browse/SW-9220
        //NOSONAR evict(to_Mem, (void *)SP_Trace_CB.base_per_hart, SP_Trace_CB.offset_per_hart);
        Log_Write(LOG_LEVEL_INFO,
                            "TRACE_RT_CONTROL:SP:Trace Disabled.\r\n");
    }

    if (dm_cmd->control & TRACE_CONTROL_TRACE_UART_ENABLE)
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
    if (dm_cmd->event_mask & TRACE_EVENT_STRING)
    {
        Log_Set_Level(dm_cmd->filter_mask & TRACE_FILTER_STRING_MASK);
    }
    Log_Write(LOG_LEVEL_INFO, "TRACE_CONFIG:SP:Trace Event/Filter Mask set.\r\n");
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

    /* Init the trace buffer lock to released state */
    Trace_Mutex_Handle = xSemaphoreCreateMutexStatic(&Trace_Mutex_Buffer);

    if (!Trace_Mutex_Handle)
    {
        Log_Write(LOG_LEVEL_ERROR, "Trace buffer Mutex creation failed!\n");
        return;
    }

    /* If init information is NULL then do default initialization. */
    if (sp_init_info == NULL)
    {
        /* Populate default Trace configurations for Service Processor. */
#if TEST_FRAMEWORK
        /* The scratch pad buffer is also used by the BL2 to hold the DDR firmware.
           During the DDR init the logging to trace buffer is disabled and therefore
           it should not cause any problem. log is redirected to trace buffer after init
           is complete.
        */
        void *trace_buff = get_scratch_buffer(&sp_init_info_l.buffer_size);
        sp_init_info_l.buffer = (uint64_t)trace_buff;
#else
        sp_init_info_l.buffer        = SP_TRACE_BUFFER_BASE;
        sp_init_info_l.buffer_size   = SP_TRACE_BUFFER_SIZE;
#endif
        sp_init_info_l.event_mask    = TRACE_EVENT_STRING;
        sp_init_info_l.filter_mask   = TRACE_EVENT_STRING_WARNING;
        sp_init_info_l.threshold     = sp_init_info_l.buffer_size;
    }
    else
    {
        memcpy(&sp_init_info_l, sp_init_info, sizeof(struct trace_init_info_t));
    }

    /* Common buffer for all SP HART. */
    SP_Trace_CB.size_per_hart = sp_init_info_l.buffer_size;
    SP_Trace_CB.base_per_hart = sp_init_info_l.buffer;

    /* Initialize Trace for each all Harts in Service Processor. */
    Trace_Init(&sp_init_info_l, &SP_Trace_CB, TRACE_STD_HEADER);

    /* Initialize trace buffer header. */
    struct trace_buffer_std_header_t *trace_header =
        (struct trace_buffer_std_header_t *)SP_Trace_CB.base_per_hart;

    /* Put the buffer type */
    trace_header->type = TRACE_SP_BUFFER;

    /* Put the MAJIC. */
    trace_header->magic_header = TRACE_MAGIC_HEADER;

    /* Put the buffer partitioning info for a single buffer. */
    trace_header->sub_buffer_count = 1;
    trace_header->sub_buffer_size = SP_TRACE_BUFFER_SIZE;

    /* populate Trace layout version in Header. */
    trace_header->version.major = TRACE_VERSION_MAJOR;
    trace_header->version.minor = TRACE_VERSION_MINOR;
    trace_header->version.patch = TRACE_VERSION_PATCH;

    /* Put the data size. */
    trace_header->data_size = sizeof(struct trace_buffer_std_header_t);
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
