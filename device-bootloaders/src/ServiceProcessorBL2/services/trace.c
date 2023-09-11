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
        Trace_Reset_SP_Dev_Stats_Buffer
        Trace_Run_Control_SP_Dev_Stats

*/
/***********************************************************************/

#include "config/mgmt_build_config.h"
#include "bl2_scratch_buffer.h"
#include "bl2_timer.h"
#include "bl_error_code.h"
#include "interrupt.h"
#include "FreeRTOS.h"
#include "portmacro.h"
#include "semphr.h"
#include "task.h"
#include <string.h>

/* Encoder function prototypes */
static inline void et_trace_buffer_lock_acquire(void);
static inline void et_trace_buffer_lock_release(void);

#define ET_TRACE_GET_TIMESTAMP() timer_get_ticks_count()
#define ET_TRACE_STRING_MAX_SIZE 128

/* Define for Encoder */
#define ET_TRACE_ENCODER_IMPL
#include "trace.h"
#include "bl2_exception.h"
#include "sp_host_iface.h"

/*
 * Service Processor Trace control block.
 */
struct trace_control_block_t SP_Trace_CB = { 0 };

/*
 * Exception Trace control block.
 */
struct trace_control_block_t SP_Exp_Trace_CB = { 0 };

/*
 * Dev Stats Trace control block.
 */
struct trace_control_block_t SP_Stats_Trace_CB = { 0 };

/* Trace buffer lock */
static SemaphoreHandle_t Trace_Mutex_Handle = NULL;
static StaticSemaphore_t Trace_Mutex_Buffer;

static void Trace_Run_Control(trace_enable_e state);

/* Trace buffer locking routines */
static inline void et_trace_buffer_lock_acquire(void)
{
    /* Acquire the Mutex (only in case of non-trap context)*/
    if (!INT_Is_Trap_Context() && Trace_Mutex_Handle)
    {
        xSemaphoreTake(Trace_Mutex_Handle, portMAX_DELAY);
    }
}

static inline void et_trace_buffer_lock_release(void)
{
    /* Release the Mutex (only in case of non-trap context)*/
    if (!INT_Is_Trap_Context() && Trace_Mutex_Handle)
    {
        xSemaphoreGive(Trace_Mutex_Handle);
    }
}

static inline void et_trace_threshold_notify(const struct trace_control_block_t *cb)
{
    struct event_message_t message;
    const struct trace_buffer_std_header_t *trace_header =
        (const struct trace_buffer_std_header_t *)cb->base_per_hart;

    /* Generate trace buffer threshold event */
    message.header.msg_id = DM_EVENT_SP_TRACE_BUFFER_FULL;
    message.header.size = sizeof(struct event_message_t) - sizeof(struct cmn_header_t);
    FILL_EVENT_PAYLOAD(&message.payload, WARNING, 1, trace_header->type, cb->threshold)

    /* Post event to Host - ignore status to avoid potential nested locks */
    SP_Host_Iface_CQ_Push_Cmd((void *)&message, sizeof(struct event_message_t));
}

/* Stats Trace buffer lock. NOTE: This lock is meant to be used within trace component */
static SemaphoreHandle_t Trace_Stats_Cb_Mutex_Handle = NULL;
static StaticSemaphore_t Trace_Stats_Cb_Mutex_Buffer;

/* Stats Trace buffer locking routines */
static inline void et_trace_stats_cb_lock_acquire(void)
{
    /* Acquire the Mutex (only in case of non-trap context)*/
    if (!INT_Is_Trap_Context() && Trace_Stats_Cb_Mutex_Handle)
    {
        xSemaphoreTake(Trace_Stats_Cb_Mutex_Handle, portMAX_DELAY);
    }
}

static inline void et_trace_stats_cb_lock_release(void)
{
    /* Release the Mutex (only in case of non-trap context)*/
    if (!INT_Is_Trap_Context() && Trace_Stats_Cb_Mutex_Handle)
    {
        xSemaphoreGive(Trace_Stats_Cb_Mutex_Handle);
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
        et_trace_buffer_lock_acquire();
        trace_header->data_size = sizeof(struct trace_buffer_std_header_t);
        SP_Trace_CB.offset_per_hart = sizeof(struct trace_buffer_std_header_t);
        et_trace_buffer_lock_release();
    }

    /* Check flag to Enable/Disable Trace. */
    if (dm_cmd->control & TRACE_CONTROL_TRACE_ENABLE)
    {
        Trace_Run_Control(TRACE_ENABLE);
        Log_Write(LOG_LEVEL_INFO, "TRACE_RT_CONTROL:SP:Trace Enabled.\r\n");
    }
    else
    {
        Trace_Run_Control(TRACE_DISABLE);
        Log_Write(LOG_LEVEL_INFO, "TRACE_RT_CONTROL:SP:Trace Disabled.\r\n");
    }

    if (dm_cmd->control & TRACE_CONTROL_TRACE_UART_ENABLE)
    {
        Log_Set_Interface(LOG_DUMP_TO_UART);
        Log_Write(LOG_LEVEL_INFO, "TRACE_RT_CONTROL:SP:Logs redirected to UART.\r\n");
    }
    else
    {
        Log_Set_Interface(LOG_DUMP_TO_TRACE);
        Log_Write(LOG_LEVEL_INFO, "TRACE_RT_CONTROL:SP:Logs redirected to Trace buffer\r\n");
    }
}

static void send_trace_control_response(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time)
{
    struct device_mgmt_default_rsp_t dm_rsp;

    FILL_RSP_HEADER(dm_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    dm_rsp.payload = DM_STATUS_SUCCESS;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "send_trace_control_response: Cqueue push error!\n");
    }
}

void Trace_Process_Config_Cmd(void *buffer)
{
    struct device_mgmt_trace_config_cmd_t *dm_cmd = (struct device_mgmt_trace_config_cmd_t *)buffer;
    struct trace_config_info_t config_info = { .filter_mask = dm_cmd->filter_mask,
                                               .event_mask = dm_cmd->event_mask };
    int32_t status = STATUS_SUCCESS;

    et_trace_buffer_lock_acquire();
    config_info.threshold = SP_Trace_CB.size_per_hart;
    status = Trace_Config(&config_info, Trace_Get_SP_CB());
    et_trace_buffer_lock_release();

    if (status != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "TRACE_CONFIG: Unable to configure Trace\n");
    }
    else
    {
        if (dm_cmd->event_mask & TRACE_EVENT_STRING)
        {
            Log_Set_Level(dm_cmd->filter_mask & TRACE_FILTER_STRING_MASK);
        }
        Log_Write(LOG_LEVEL_INFO, "TRACE_CONFIG:SP:Trace Event/Filter Mask set.\r\n");
    }
}

static void send_trace_config_response(tag_id_t tag_id, msg_id_t msg_id, uint64_t req_start_time)
{
    struct device_mgmt_default_rsp_t dm_rsp;

    FILL_RSP_HEADER(dm_rsp, tag_id, msg_id, timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    dm_rsp.payload = DM_STATUS_SUCCESS;

    if (0 !=
        SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_trace_config_rsp_t)))
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
*       status of function call success or error
*
***********************************************************************/
int32_t Trace_Init_SP(const struct trace_init_info_t *sp_init_info)
{
    struct trace_init_info_t sp_init_info_l;
    int32_t status = ERROR_INVALID_ARGUMENT;

    /* Init the trace buffer lock to released state */
    Trace_Mutex_Handle = xSemaphoreCreateMutexStatic(&Trace_Mutex_Buffer);

    if (!Trace_Mutex_Handle)
    {
        Log_Write(LOG_LEVEL_ERROR, "Trace buffer Mutex creation failed!\n");
        return ERROR_INVALID_ARGUMENT;
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
        sp_init_info_l.buffer = SP_TRACE_BUFFER_BASE;
        sp_init_info_l.buffer_size = SP_TRACE_BUFFER_SIZE_PER_SUB_BUFFER;
#endif
        sp_init_info_l.event_mask = TRACE_EVENT_STRING;
        sp_init_info_l.filter_mask = TRACE_EVENT_STRING_WARNING;
        sp_init_info_l.threshold = sp_init_info_l.buffer_size;
    }
    else
    {
        memcpy(&sp_init_info_l, sp_init_info, sizeof(struct trace_init_info_t));
    }

    /* Register locks for SP trace */
    SP_Trace_CB.buffer_lock_acquire = et_trace_buffer_lock_acquire;
    SP_Trace_CB.buffer_lock_release = et_trace_buffer_lock_release;

    /* Register trace threshold notification event handler */
    SP_Trace_CB.threshold_notify = et_trace_threshold_notify;

    /* Common buffer for all SP HART. */
    SP_Trace_CB.size_per_hart = sp_init_info_l.buffer_size;
    SP_Trace_CB.base_per_hart = sp_init_info_l.buffer;

    /* Initialize Trace for each all Harts in Service Processor. */
    status = Trace_Init(&sp_init_info_l, &SP_Trace_CB, TRACE_STD_HEADER);

    if (status == SUCCESS)
    {
        /* Initialize trace buffer header. */
        struct trace_buffer_std_header_t *trace_header =
            (struct trace_buffer_std_header_t *)SP_Trace_CB.base_per_hart;

        /* Put the buffer type */
        trace_header->type = TRACE_SP_BUFFER;

        /* Put the MAJIC. */
        trace_header->magic_header = TRACE_MAGIC_HEADER;

        /* SP buffer has been partitioned into two parts, one will be used for SP events and other
        will be used explicitly for exception events
        Put the buffer partitioning info for a single buffer. */
        trace_header->sub_buffer_count = SP_TRACE_SUB_BUFFER_COUNT;
        trace_header->sub_buffer_size = SP_TRACE_BUFFER_SIZE_PER_SUB_BUFFER;

        /* populate Trace layout version in Header. */
        trace_header->version.major = TRACE_VERSION_MAJOR;
        trace_header->version.minor = TRACE_VERSION_MINOR;
        trace_header->version.patch = TRACE_VERSION_PATCH;

        /* Put the data size. */
        trace_header->data_size = sizeof(struct trace_buffer_std_header_t);
        ETSOC_MEM_EVICT((uint64_t *)SP_TRACE_BUFFER_BASE, SP_Trace_CB.offset_per_hart, to_L2)
    }

    return status;
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
struct trace_control_block_t *Trace_Get_SP_CB(void)
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
    if (state == TRACE_DISABLE)
    {
        Trace_Update_SP_Buffer_Header();
    }
    SP_Trace_CB.enable = state;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Update_SP_Buffer_Header
*
*   DESCRIPTION
*
*       This function Updates Trace buffer header to reflect current data
*       in buffer.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Update_SP_Buffer_Header(void)
{
    struct trace_buffer_std_header_t *trace_header =
        (struct trace_buffer_std_header_t *)SP_Trace_CB.base_per_hart;

    et_trace_buffer_lock_acquire();

    trace_header->data_size = SP_Trace_CB.offset_per_hart;
    ETSOC_MEM_EVICT((uint64_t *)SP_TRACE_BUFFER_BASE, SP_Trace_CB.offset_per_hart, to_L2)

    et_trace_buffer_lock_release();
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

/************************************************************************
*
*   FUNCTION
*
*       Trace_Exception_Init_SP
*
*   DESCRIPTION
*
*       This function initializes Trace for Service Processor exception reporting
*
*   INPUTS
*
*       init_info exception trace init info
*
*   OUTPUTS
*
*       Status of SP Exception trace initialization (Success or any Error)
*
***********************************************************************/
int32_t Trace_Exception_Init_SP(const struct trace_init_info_t *init_info)
{
    struct trace_init_info_t exp_init_info_l;

    if (init_info == NULL)
    {
        /* Populate default Trace configurations for Service Processor. */
        exp_init_info_l.event_mask = TRACE_EVENT_STRING;
        exp_init_info_l.filter_mask = TRACE_EVENT_STRING_WARNING;
        exp_init_info_l.threshold = SP_TRACE_BUFFER_SIZE_PER_SUB_BUFFER;
    }
    else
    {
        memcpy(&exp_init_info_l, init_info, sizeof(struct trace_init_info_t));
    }

    /* Initialize exception trace buffer as sub-buffer of SP trace buffer
       with size equal to sub buffer size. */
    SP_Exp_Trace_CB.size_per_hart = SP_TRACE_BUFFER_SIZE_PER_SUB_BUFFER;
    SP_Exp_Trace_CB.base_per_hart = (SP_TRACE_BUFFER_BASE + SP_TRACE_BUFFER_SIZE_PER_SUB_BUFFER);

    struct trace_buffer_size_header_t *size_header =
        (struct trace_buffer_size_header_t *)SP_Exp_Trace_CB.base_per_hart;

    size_header->data_size = sizeof(struct trace_buffer_size_header_t);

    /* Set the default offset */
    SP_Exp_Trace_CB.offset_per_hart = sizeof(struct trace_buffer_size_header_t);
    ETSOC_MEM_EVICT((uint64_t *)SP_Exp_Trace_CB.base_per_hart, SP_Exp_Trace_CB.offset_per_hart,
                    to_L2)
    /* Buffer locks for exception buffer are not required as this will only be accessed
       from exception reporting context. */
    SP_Exp_Trace_CB.buffer_lock_acquire = NULL;
    SP_Exp_Trace_CB.buffer_lock_release = NULL;

    /* Register trace threshold notification event handler */
    SP_Exp_Trace_CB.threshold_notify = et_trace_threshold_notify;

    /* Initialize Trace for SP Exception buffer. */
    return Trace_Init(&exp_init_info_l, &SP_Exp_Trace_CB, TRACE_SIZE_HEADER);
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Get_SP_Exp_CB
*
*   DESCRIPTION
*
*       This function returns the Trace control block (CB) for SP Exception.
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
struct trace_control_block_t *Trace_Get_SP_Exp_CB(void)
{
    return &SP_Exp_Trace_CB;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Exception_Dump_Context
*
*   DESCRIPTION
*
*       This function dumps stack context to trace and returns trace buffer
        offset.
*
*   INPUTS
*
*       stack_frame
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
uint8_t *Trace_Exception_Dump_Context(const void *stack_frame)
{
    struct dev_context_registers_t context = { 0 };
    const uint64_t *stack_pointer = (const uint64_t *)stack_frame;
    uint8_t *trace_buf;

    /* Dump the stack frame - for stack frame defintion,
        see the comments in the portASM.c file */

    /* Move the stack pointer to x1 saved location */
    stack_pointer++;

    /* Save x1 first */
    context.gpr[0] = *stack_pointer;

    /* Move the stack pointer to x5 saved location */
    stack_pointer++;

    /* Dump x5 to x31 to specified context structure */
    memcpy((void *)&context.gpr[4], stack_pointer,
           SP_EXCEPTION_STACK_FRAME_SIZE - (sizeof(uint64_t) * 1));

    /* Dump CSRs to the specified context structure */
    asm volatile("csrr %0, mcause\n"
                 "csrr %1, mstatus\n"
                 "csrr %2, mepc\n"
                 "csrr %3, mtval"
                 : "=r"(context.cause), "=r"(context.status), "=r"(context.epc),
                   "=r"(context.tval));

    /* Log the execution stack event to trace */
    trace_buf = Trace_Execution_Stack(&SP_Exp_Trace_CB, &context);

    /* Update data size in size header of exception buffer */
    struct trace_buffer_size_header_t *size_header =
        (struct trace_buffer_size_header_t *)SP_Exp_Trace_CB.base_per_hart;

    size_header->data_size = SP_Exp_Trace_CB.offset_per_hart;
    ETSOC_MEM_EVICT((uint64_t *)SP_Exp_Trace_CB.base_per_hart, SP_Exp_Trace_CB.offset_per_hart,
                    to_L2)
    return trace_buf;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Run_Control_SP_Dev_Stats()
*
*   DESCRIPTION
*
*       This function enable/disable Trace for Service Processor stats buffer.
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
void Trace_Run_Control_SP_Dev_Stats(trace_enable_e state)
{
    if (state == TRACE_DISABLE)
    {
        Trace_Update_SP_Stats_Buffer_Header();
    }
    SP_Stats_Trace_CB.enable = state;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Reset_SP_Dev_Stats_Buffer
*
*   DESCRIPTION
*
*       This function resets Service Processor stats trace buffer.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Reset_SP_Dev_Stats_Buffer(void)
{
    struct trace_buffer_std_header_t *trace_header =
        (struct trace_buffer_std_header_t *)SP_Stats_Trace_CB.base_per_hart;

    /* Reset the trace buffer */
    et_trace_stats_cb_lock_acquire();
    trace_header->data_size = sizeof(struct trace_buffer_std_header_t);
    SP_Stats_Trace_CB.offset_per_hart = sizeof(struct trace_buffer_std_header_t);
    ETSOC_MEM_EVICT((uint64_t *)SP_STATS_TRACE_BUFFER_BASE, SP_Stats_Trace_CB.offset_per_hart,
                    to_L2)
    et_trace_stats_cb_lock_release();
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Init_SP_Dev_Stats
*
*   DESCRIPTION
*
*       This function initializes Trace for device stats.
*       NOTE: SP Stats Trace Buffer is not thread safe.
*
*   INPUTS
*
*       init_info exception trace init info
*
*   OUTPUTS
*
*       Status of SP Exception trace initialization (Success or any Error)
*
***********************************************************************/
int32_t Trace_Init_SP_Dev_Stats(const struct trace_init_info_t *dev_trace_init_info)
{
    int32_t status = ERROR_INVALID_ARGUMENT;
    struct trace_init_info_t dev_trace_init_info_l;

    /* Init the stats trace cb lock to released state */
    Trace_Stats_Cb_Mutex_Handle = xSemaphoreCreateMutexStatic(&Trace_Stats_Cb_Mutex_Buffer);

    /* If init information is NULL then do default initialization. */
    if (dev_trace_init_info == NULL)
    {
        /* Populate default Trace configurations for Service Processor. */

        dev_trace_init_info_l.buffer = SP_STATS_TRACE_BUFFER_BASE;
        dev_trace_init_info_l.buffer_size = SP_STATS_BUFFER_SIZE;
        dev_trace_init_info_l.event_mask = TRACE_EVENT_STRING;
        dev_trace_init_info_l.filter_mask = TRACE_EVENT_STRING_DEBUG;
        dev_trace_init_info_l.threshold = dev_trace_init_info_l.buffer_size;
    }
    else
    {
        memcpy(&dev_trace_init_info_l, dev_trace_init_info, sizeof(struct trace_init_info_t));
    }

    /* Common buffer for all SP HART. */
    SP_Stats_Trace_CB.size_per_hart = dev_trace_init_info_l.buffer_size;
    SP_Stats_Trace_CB.base_per_hart = dev_trace_init_info_l.buffer;

    /* Trace buffer locks are not required as only stats task will be accessing it*/
    SP_Stats_Trace_CB.buffer_lock_acquire = NULL;
    SP_Stats_Trace_CB.buffer_lock_release = NULL;

    /* Threshold notification not set for now */
    SP_Stats_Trace_CB.threshold_notify = NULL;

    /* Initialize Trace for each all Harts in Service Processor. */
    status = Trace_Init(&dev_trace_init_info_l, &SP_Stats_Trace_CB, TRACE_STD_HEADER);

    /* Initialize trace buffer header. */
    struct trace_buffer_std_header_t *trace_header =
        (struct trace_buffer_std_header_t *)SP_Stats_Trace_CB.base_per_hart;

    /* Put the buffer type */
    trace_header->type = TRACE_SP_STATS_BUFFER;

    /* Put the MAGIC. */
    trace_header->magic_header = TRACE_MAGIC_HEADER;
    trace_header->sub_buffer_count = SP_DEV_STATS_TRACE_SUB_BUFFER_COUNT;
    trace_header->sub_buffer_size = SP_STATS_BUFFER_SIZE;

    /* populate Trace layout version in Header. */
    trace_header->version.major = TRACE_VERSION_MAJOR;
    trace_header->version.minor = TRACE_VERSION_MINOR;
    trace_header->version.patch = TRACE_VERSION_PATCH;

    /* Put the data size. */
    trace_header->data_size = sizeof(struct trace_buffer_std_header_t);
    ETSOC_MEM_EVICT((uint64_t *)SP_STATS_TRACE_BUFFER_BASE, SP_Stats_Trace_CB.offset_per_hart,
                    to_L2)
    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Get_Dev_Stats_CB
*
*   DESCRIPTION
*
*       This function returns the Trace control block (CB) for Dev Stats.
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
struct trace_control_block_t *Trace_Get_Dev_Stats_CB(void)
{
    return &SP_Stats_Trace_CB;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Update_SP_Stats_Buffer_Header
*
*   DESCRIPTION
*
*       This function Updates Trace buffer header to reflect current data
*       in buffer.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Update_SP_Stats_Buffer_Header(void)
{
    struct trace_buffer_std_header_t *trace_header =
        (struct trace_buffer_std_header_t *)SP_Stats_Trace_CB.base_per_hart;

    /* Update data size in trace header */
    et_trace_stats_cb_lock_acquire();
    trace_header->data_size = SP_Stats_Trace_CB.offset_per_hart;
    ETSOC_MEM_EVICT((uint64_t *)SP_STATS_TRACE_BUFFER_BASE, SP_Stats_Trace_CB.offset_per_hart,
                    to_L2)
    et_trace_stats_cb_lock_release();
}
