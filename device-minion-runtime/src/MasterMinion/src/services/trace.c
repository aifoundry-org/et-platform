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
        Trace_Init_MM
        Trace_Get_MM_CB

*/
/***********************************************************************/

#include <esperanto/device-apis/operations-api/device_ops_api_spec.h>
#include <esperanto/device-apis/device_apis_trace_types.h>
#include "device-common/atomic.h"
#include "device-common/cacheops.h"
#include "device-common/hart.h"
#include "device-common/hpm_counter.h"
#include "layout.h"
#include "config/mm_config.h"
#include "services/log.h"
#include "common_trace_defs.h"
#include "etsoc_memory.h"

static inline void et_trace_write_float(void *addr, float value);

#define ET_TRACE_GET_HPM_COUNTER(id)      hpm_read_counter(id)
#define ET_TRACE_GET_TIMESTAMP()          hpm_read_counter3()
#define ET_TRACE_GET_HART_ID()            get_hart_id()

/* Master Minion Trace memory access primitives. */
#define ET_TRACE_READ_U8(addr)            atomic_load_local_8(&addr)
#define ET_TRACE_READ_U16(addr)           atomic_load_local_16(&addr)
#define ET_TRACE_READ_U32(addr)           atomic_load_local_32(&addr)
#define ET_TRACE_READ_U64(addr)           atomic_load_local_64(&addr)
#define ET_TRACE_WRITE_U8(addr, value)    atomic_store_local_8(&addr, value)
#define ET_TRACE_WRITE_U16(addr, value)   atomic_store_local_16(&addr, value)
#define ET_TRACE_WRITE_U32(addr, value)   atomic_store_local_32(&addr, value)
#define ET_TRACE_WRITE_U64(addr, value)   atomic_store_local_64(&addr, value)
#define ET_TRACE_WRITE_FLOAT(loc, value)  et_trace_write_float(&(loc), (value))
#define ET_TRACE_MEM_CPY(dest, src, size) ETSOC_Memory_Write_Local_Atomic(src, dest, size)

#define ET_TRACE_ENCODER_IMPL
#include "services/trace.h"

union data_u32_f {
    uint32_t value_u32;
    float value_f;
};

static inline void et_trace_write_float(void *addr, float value)
{
    union data_u32_f data;
    data.value_f = value;
    atomic_store_local_32(addr, data.value_u32);
}

/*! \def MM_DEFAULT_THREAD_MASK
    \brief Default masks to enable Trace for Dispatcher, SQ Worker (SQW),
        DMA Worker : Read & Write, and Kernel Worker (KW)
*/
#define MM_DEFAULT_THREAD_MASK   ((1UL << (DISPATCHER_BASE_HART_ID - MM_BASE_ID)) |              \
                                  (1UL << (DMAW_BASE_HART_ID - MM_BASE_ID)) |                    \
                                  (1UL << (DMAW_BASE_HART_ID + HARTS_PER_MINION - MM_BASE_ID)) | \
                                  (1UL << (SQW_BASE_HART_ID - MM_BASE_ID)) |                     \
                                  (1UL << (KW_BASE_HART_ID - MM_BASE_ID)))

/*
 * Master Minion Trace control block.
 */
typedef struct mm_trace_control_block {
    struct trace_control_block_t cb; /**!< Common Trace library control block. */
    uint64_t cm_shire_mask;          /**!< Compute Minion Shire mask to fetch Trace data from CM. */
    uint64_t cm_thread_mask;         /**!< Compute Minion Shire mask to fetch Trace data from CM. */
} __attribute__((aligned(64))) mm_trace_control_block_t;

/* A local Trace control block for all Master Minions. */
static mm_trace_control_block_t MM_Trace_CB =
                {.cb = {0}, .cm_shire_mask = CM_DEFAULT_TRACE_SHIRE_MASK,
                 .cm_thread_mask = CM_DEFAULT_TRACE_THREAD_MASK};

/************************************************************************
*
*   FUNCTION
*
*       Trace_Init_MM
*
*   DESCRIPTION
*
*       This function initializes Trace for all harts in Master Minion
*       Shire. This should be called once by a single MM Hart only.
*
*   INPUTS
*
*       trace_init_info_t    Trace init info for Master Minion shire.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Init_MM(const struct trace_init_info_t *mm_init_info)
{
    int8_t internal_status = STATUS_SUCCESS;
    struct trace_init_info_t hart_init_info;

    /* If init information is NULL then do default initialization. */
    if (mm_init_info == NULL)
    {
        /* Populate default Trace configurations for Master Minion. */
        hart_init_info.shire_mask    = MM_SHIRE_MASK;
        hart_init_info.thread_mask   = MM_DEFAULT_THREAD_MASK;
        hart_init_info.event_mask    = TRACE_EVENT_STRING;
        hart_init_info.filter_mask   = TRACE_EVENT_STRING_INFO;
        hart_init_info.threshold     = MM_TRACE_BUFFER_SIZE;
    }
    /* Check if shire mask is of Master Minion and atleast one thread is enabled. */
    else if ((mm_init_info->shire_mask & MM_SHIRE_MASK) && (mm_init_info->thread_mask & MM_HART_MASK))
    {
        /* Populate given init information into per-thread Trace information structure. */
        hart_init_info.shire_mask    = MM_SHIRE_MASK;
        hart_init_info.thread_mask   = MM_HART_MASK;
        hart_init_info.filter_mask   = mm_init_info->filter_mask;
        hart_init_info.event_mask    = mm_init_info->event_mask;
        hart_init_info.threshold     = mm_init_info->threshold;
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR,
            "MM:TRACE_CONFIG:Invalid Init Info.\r\n");
        MM_Trace_CB.cb.enable = TRACE_DISABLE;

        /* Trace init information is invalid. */
        internal_status = INVALID_TRACE_INIT_INFO;
    }

    if(internal_status == STATUS_SUCCESS)
    {
        /* Common buffer for all MM Harts. */
        MM_Trace_CB.cb.size_per_hart = MM_TRACE_BUFFER_SIZE;
        MM_Trace_CB.cb.base_per_hart = MM_TRACE_BUFFER_BASE;

        /* Initialize Trace for each all Harts in Master Minion. */
        Trace_Init(&hart_init_info, &MM_Trace_CB.cb, TRACE_STD_HEADER);

        /* Initialize trace buffer header. */
        struct trace_buffer_std_header_t *trace_header =
            (struct trace_buffer_std_header_t *)MM_TRACE_BUFFER_BASE;

        atomic_store_local_32(&(trace_header->magic_header), TRACE_MAGIC_HEADER);
        atomic_store_local_32(&trace_header->data_size,
                            sizeof(struct trace_buffer_std_header_t));
        atomic_store_local_16(&(trace_header->type), TRACE_MM_BUFFER);
    }

    /* Evict an updated control block to L2 memory. */
    ETSOC_MEM_EVICT(&MM_Trace_CB, sizeof(mm_trace_control_block_t), to_L2)
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Get_MM_CB
*
*   DESCRIPTION
*
*       This function returns the common Trace control block (CB) for all
*       MM Harts.
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
struct trace_control_block_t* Trace_Get_MM_CB(void)
{
    return &MM_Trace_CB.cb;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Get_CM_Shire_Mask
*
*   DESCRIPTION
*
*       This function returns shire mask of Compute Minions for which
*       Trace is enabled.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint64_t    CM Shire Mask.
*
***********************************************************************/
uint64_t Trace_Get_CM_Shire_Mask(void)
{
    return atomic_load_local_64(&MM_Trace_CB.cm_shire_mask);
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Get_CM_Thread_Mask
*
*   DESCRIPTION
*
*       This function returns thread mask of Compute Minions for which
*       Trace is enabled.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint64_t    CM Thread Mask.
*
***********************************************************************/
uint64_t Trace_Get_CM_Thread_Mask(void)
{
    return atomic_load_local_64(&MM_Trace_CB.cm_thread_mask);
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Set_CM_Shire_Mask
*
*   DESCRIPTION
*
*       This function sets shire mask of Compute Minions for which
*       Trace is enabled.
*
*   INPUTS
*
*       uint64_t    CM Shire Mask.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Set_CM_Shire_Mask(uint64_t shire_mask)
{
    atomic_store_local_64(&MM_Trace_CB.cm_shire_mask, shire_mask);
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Set_CM_Thread_Mask
*
*   DESCRIPTION
*
*       This function sets thread mask of Compute Minions for which
*       Trace is enabled.
*
*   INPUTS
*
*       uint64_t    CM Thread Mask.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Set_CM_Thread_Mask(uint64_t thread_mask)
{
    atomic_store_local_64(&MM_Trace_CB.cm_thread_mask, thread_mask);
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_RT_Control_MM
*
*   DESCRIPTION
*
*       This function updates the control of MM Trace runtime.
*
*   INPUTS
*
*       uint32_t    Bit encoded trace control flags.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_RT_Control_MM(uint32_t control)
{
    /* Check flag to reset Trace buffer. */
    if (control & TRACE_RT_CONTROL_RESET_TRACEBUF)
    {
        atomic_store_local_32(&(MM_Trace_CB.cb.offset_per_hart),
            sizeof(struct trace_buffer_std_header_t));
    }

    /* Check flag to Enable/Disable Trace. */
    if (control & TRACE_RT_CONTROL_ENABLE_TRACE)
    {
        Trace_Set_Enable_MM(TRACE_ENABLE);
        Log_Write(LOG_LEVEL_DEBUG,
                "TRACE_RT_CONTROL:MM:Trace Enabled.\r\n");
    }
    else
    {
        Trace_Set_Enable_MM(TRACE_DISABLE);
        Log_Write(LOG_LEVEL_DEBUG,
                "TRACE_RT_CONTROL:MM:Trace Disabled.\r\n");
    }

    /* Check flag to redirect logs to Trace or UART. */
    if (control & TRACE_RT_CONTROL_LOG_TO_UART)
    {
        Log_Set_Interface(LOG_DUMP_TO_UART);
        Log_Write(LOG_LEVEL_DEBUG,
                "TRACE_RT_CONTROL:MM:Logs redirected to UART.\r\n");
    }
    else
    {
        Log_Set_Interface(LOG_DUMP_TO_TRACE);
        Log_Write(LOG_LEVEL_DEBUG,
                "TRACE_RT_CONTROL:MM:Logs redirected to Trace buffer.\r\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Evict_Buffer_MM
*
*   DESCRIPTION
*
*       This function Evict the MM Trace buffer upto current used buffer,
*       it also updates the trace buffer header to include buffer usage.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint32_t    Size of buffer that was used and evicted.
*
***********************************************************************/
uint32_t Trace_Evict_Buffer_MM(void)
{
    struct trace_buffer_std_header_t *trace_header =
        (struct trace_buffer_std_header_t *)MM_TRACE_BUFFER_BASE;
    uint32_t offset = atomic_load_local_32(&(MM_Trace_CB.cb.offset_per_hart));

    /* Store used buffer size in buffer header. */
    atomic_store_local_32(&trace_header->data_size, offset);

    ETSOC_MEM_EVICT((uint64_t *)MM_TRACE_BUFFER_BASE, offset, to_Mem)

    return offset;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Set_Enable_MM
*
*   DESCRIPTION
*
*       This function enables/disables Trace for Master Minion. If its
*       Trace disable command then it will also evict the Trace buffer
*       to L3 Cache.
*
*   INPUTS
*
*       trace_enable_e  Enum to Enable/Disable Trace.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Set_Enable_MM(trace_enable_e control)
{
    atomic_store_local_8(&(MM_Trace_CB.cb.enable), control);

    if (control == TRACE_DISABLE)
    {
        Trace_Evict_Buffer_MM();
    }
}
