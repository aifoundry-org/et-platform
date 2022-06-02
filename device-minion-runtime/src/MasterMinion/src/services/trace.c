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
        Trace_Configure_MM
        Trace_Get_MM_CB
        Trace_Get_CM_Shire_Mask
        Trace_Get_CM_Thread_Mask
        Trace_Configure_CM_RT
        Trace_RT_Control_MM
        Trace_Evict_Buffer_MM

*/
/***********************************************************************/

/* common-api, device_ops_api */
#include <esperanto/device-apis/operations-api/device_ops_api_spec.h>

/* mm_rt_svcs */
#include <etsoc/drivers/pmu/pmu.h>
#include <etsoc/isa/atomic.h>
#include <etsoc/isa/cacheops.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/etsoc_memory.h>
#include <etsoc/isa/sync.h>
#include <system/layout.h>

/* mm specific headers */
#include "config/mm_config.h"
#include "services/cm_iface.h"
#include "workers/cw.h"
#include "services/log.h"
#include "services/host_cmd_hdlr.h"

/* mm_rt_helpers */
#include "cm_mm_defines.h"
#include "error_codes.h"

/* Encoder function prototypes */
static inline void et_trace_write_float(void *addr, float value);
void et_trace_mm_cb_lock_acquire(void);
void et_trace_mm_cb_lock_release(void);

#define ET_TRACE_GET_HPM_COUNTER(id) pmu_core_counter_read_unpriv(id)
#define ET_TRACE_GET_TIMESTAMP()     PMC_Get_Current_Cycles()
#define ET_TRACE_GET_HART_ID()       get_hart_id()

/* Master Minion Trace memory access primitives. */
#define ET_TRACE_READ_U8(addr)            atomic_load_local_8(&addr)
#define ET_TRACE_READ_U16(addr)           atomic_load_local_16(&addr)
#define ET_TRACE_READ_U32(addr)           atomic_load_local_32(&addr)
#define ET_TRACE_READ_U64(addr)           atomic_load_local_64(&addr)
#define ET_TRACE_READ_U64_PTR(addr)       atomic_load_local_64((void *)&addr)
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
#define MM_DEFAULT_THREAD_MASK                                                                     \
    ((1UL << (DISPATCHER_BASE_HART_ID - MM_BASE_ID)) | (1UL << (DMAW_BASE_HART_ID - MM_BASE_ID)) | \
        (1UL << (DMAW_BASE_HART_ID + HARTS_PER_MINION - MM_BASE_ID)) |                             \
        (1UL << (SQW_BASE_HART_ID - MM_BASE_ID)) | (1UL << (KW_BASE_HART_ID - MM_BASE_ID)))

/*
 * Master Minion Trace control block.
 */
typedef struct mm_trace_control_block {
    struct trace_control_block_t cb; /**!< Common Trace library control block. */
    uint64_t cm_shire_mask;          /**!< Compute Minion Shire mask to fetch Trace data from CM. */
    uint64_t cm_thread_mask;         /**!< Compute Minion Shire mask to fetch Trace data from CM. */
    spinlock_t mm_trace_cb_lock;     /**!< Lock to serialize operations on MM trace CB */
    spinlock_t
        trace_internal_cb_lock; /**!< Lock to serialize operation on CB internal fields other than MM CB (it has its own lock). */
} __attribute__((aligned(64))) mm_trace_control_block_t;

/* A local Trace control block for all Master Minions. */
static mm_trace_control_block_t MM_Trace_CB = { .cb = { 0 },
    .cm_shire_mask = CM_DEFAULT_TRACE_SHIRE_MASK,
    .cm_thread_mask = CM_DEFAULT_TRACE_THREAD_MASK };

/* A local Trace control block for Master Minion Dev Stats. */
static mm_trace_control_block_t MM_Stats_Trace_CB = { .cb = { 0 } };

/* Trace buffer locking routines
   WARNING: This lock is shared with Trace encoder, So while using this in MM Trace component,
   there should be no Trace logs used after acquiring this. */
void et_trace_mm_cb_lock_acquire(void)
{
    /* Acquire the lock */
    acquire_local_spinlock(&MM_Trace_CB.mm_trace_cb_lock);
}

void et_trace_mm_cb_lock_release(void)
{
    /* Release the lock */
    release_local_spinlock(&MM_Trace_CB.mm_trace_cb_lock);
}

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
*       int32_t           Successful status or error code.
*
***********************************************************************/
int32_t Trace_Init_MM(const struct trace_init_info_t *mm_init_info)
{
    int32_t status = STATUS_SUCCESS;
    struct trace_init_info_t hart_init_info;

    /* If init information is NULL then do default initialization. */
    if (mm_init_info == NULL)
    {
        /* Populate default Trace configurations for Master Minion. */
        hart_init_info.shire_mask = MM_SHIRE_MASK;
        hart_init_info.thread_mask = MM_DEFAULT_THREAD_MASK;
        hart_init_info.event_mask = TRACE_EVENT_STRING;
        /* Set Trace default log level to Debug. This log level is independent from Log component. */
        hart_init_info.filter_mask = TRACE_EVENT_STRING_DEBUG;
        hart_init_info.threshold = MM_TRACE_BUFFER_SIZE;
    }
    /* Check if shire mask is of Master Minion and atleast one thread is enabled. */
    else if (!(mm_init_info->shire_mask & MM_SHIRE_MASK))
    {
        Log_Write(LOG_LEVEL_ERROR, "MM:TRACE_CONFIG:Invalid Init Info.\r\n");
        MM_Trace_CB.cb.enable = TRACE_DISABLE;

        status = TRACE_ERROR_INVALID_SHIRE_MASK;
    }
    else if (!(mm_init_info->thread_mask & MM_HART_MASK))
    {
        Log_Write(LOG_LEVEL_ERROR, "MM:TRACE_CONFIG:Invalid Init Info.\r\n");
        MM_Trace_CB.cb.enable = TRACE_DISABLE;

        status = TRACE_ERROR_INVALID_THREAD_MASK;
    }
    else
    {
        /* Populate given init information into per-thread Trace information structure. */
        hart_init_info.shire_mask = MM_SHIRE_MASK;
        hart_init_info.thread_mask = MM_HART_MASK;
        hart_init_info.filter_mask = mm_init_info->filter_mask;
        hart_init_info.event_mask = mm_init_info->event_mask;
        hart_init_info.threshold = mm_init_info->threshold;
    }

    if (status == STATUS_SUCCESS)
    {
        /* Initialize the spinlock */
        init_local_spinlock(&MM_Trace_CB.mm_trace_cb_lock, 0);
        init_local_spinlock(&MM_Trace_CB.trace_internal_cb_lock, 0);

        /* Common buffer for all MM Harts. */
        MM_Trace_CB.cb.size_per_hart = MM_TRACE_BUFFER_SIZE;
        MM_Trace_CB.cb.base_per_hart = MM_TRACE_BUFFER_BASE;

        /* Register locks for MM trace */
        MM_Trace_CB.cb.buffer_lock_acquire = et_trace_mm_cb_lock_acquire;
        MM_Trace_CB.cb.buffer_lock_release = et_trace_mm_cb_lock_release;

        /* Initialize Trace for each all Harts in Master Minion. */
        status = Trace_Init(&hart_init_info, &MM_Trace_CB.cb, TRACE_STD_HEADER);

        if (status != STATUS_SUCCESS)
        {
            status = TRACE_ERROR_MM_TRACE_CONFIG_FAILED;
        }

        /* Initialize trace buffer header. */
        struct trace_buffer_std_header_t *trace_header =
            (struct trace_buffer_std_header_t *)MM_TRACE_BUFFER_BASE;

        /* Update trace buffer header for buffer layout version and partitioning information.
           One common buffer is used by all Harts to log Tracing. */
        trace_header->magic_header = TRACE_MAGIC_HEADER;
        trace_header->type = TRACE_MM_BUFFER;
        trace_header->data_size = sizeof(struct trace_buffer_std_header_t);
        trace_header->sub_buffer_size = MM_TRACE_BUFFER_SIZE;
        trace_header->sub_buffer_count = 1;
        trace_header->version.major = TRACE_VERSION_MAJOR;
        trace_header->version.minor = TRACE_VERSION_MINOR;
        trace_header->version.patch = TRACE_VERSION_PATCH;

        ETSOC_MEM_EVICT((void *)trace_header, sizeof(struct trace_buffer_std_header_t), to_L3)
    }

    /* Evict an updated control block to L2 memory. */
    ETSOC_MEM_EVICT(&MM_Trace_CB, sizeof(mm_trace_control_block_t), to_L2)

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Configure_MM
*
*   DESCRIPTION
*
*       This function configures the Trace.
*       NOTE:Trace must be initialized using Trace_Init_MM() before this
*       function.
*
*   INPUTS
*
*       trace_config_info_t    Trace init info for Master Minion shire.
*
*   OUTPUTS
*
*       int32_t           Successful status or error code.
*
***********************************************************************/
int32_t Trace_Configure_MM(const struct trace_config_info_t *mm_config_info)
{
    int32_t status = TRACE_ERROR_INVALID_TRACE_CONFIG_INFO;

    /* Check if init information pointer is NULL. */
    if (mm_config_info != NULL)
    {
        /* Acquire the lock */
        et_trace_mm_cb_lock_acquire();

        status = Trace_Config(mm_config_info, &MM_Trace_CB.cb);

        /* Release the lock */
        et_trace_mm_cb_lock_release();

        if (status != STATUS_SUCCESS)
        {
            status = TRACE_ERROR_MM_TRACE_CONFIG_FAILED;
        }
    }

    return status;
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
struct trace_control_block_t *Trace_Get_MM_CB(void)
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
*       Trace_Configure_CM_RT
*
*   DESCRIPTION
*
*       This function configures the CM runtime tracing by sending a MM to
*       CM message.
*
*   INPUTS
*
*       config_msg    Pointer to CM config message info
*
*   OUTPUTS
*
*       int32_t        status success or error
*
***********************************************************************/
int32_t Trace_Configure_CM_RT(mm_to_cm_message_trace_rt_config_t *config_msg)
{
    int32_t status;

    /* Acquire the lock */
    acquire_local_spinlock(&MM_Trace_CB.trace_internal_cb_lock);

    /* Transmit the message to Compute Minions */
    status = CM_Iface_Multicast_Send(CW_Get_Booted_Shires(), (cm_iface_message_t *)config_msg);

    /* Release the lock */
    release_local_spinlock(&MM_Trace_CB.trace_internal_cb_lock);

    /* Save the configured values in MM CB */
    if (status == STATUS_SUCCESS)
    {
        atomic_store_local_64(&MM_Trace_CB.cm_shire_mask, config_msg->shire_mask);
        atomic_store_local_64(&MM_Trace_CB.cm_thread_mask, config_msg->thread_mask);
    }

    return status;
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
    /* Acquire the lock */
    et_trace_mm_cb_lock_acquire();

    /* Check flag to reset Trace buffer. */
    if (control & TRACE_RT_CONTROL_RESET_TRACEBUF)
    {
        atomic_store_local_32(
            &(MM_Trace_CB.cb.offset_per_hart), sizeof(struct trace_buffer_std_header_t));
    }

    /* Check flag to Enable/Disable Trace. */
    if (control & TRACE_RT_CONTROL_ENABLE_TRACE)
    {
        atomic_store_local_8(&(MM_Trace_CB.cb.enable), TRACE_ENABLE);
    }
    else
    {
        atomic_store_local_8(&(MM_Trace_CB.cb.enable), TRACE_DISABLE);
    }

    /* Release the lock */
    et_trace_mm_cb_lock_release();

    /* Check flag to redirect logs to Trace or UART. */
    if (control & TRACE_RT_CONTROL_LOG_TO_UART)
    {
        Log_Set_Interface(LOG_DUMP_TO_UART);
        Log_Write(LOG_LEVEL_DEBUG, "TRACE_RT_CONTROL:MM:Logs redirected to UART.\r\n");
    }
    else
    {
        Log_Set_Interface(LOG_DUMP_TO_TRACE);
        Log_Write(LOG_LEVEL_DEBUG, "TRACE_RT_CONTROL:MM:Logs redirected to Trace buffer.\r\n");
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
    et_trace_mm_cb_lock_acquire();
    uint32_t offset = atomic_load_local_32(&(MM_Trace_CB.cb.offset_per_hart));

    /* Store used buffer size in buffer header. */
    atomic_store_local_32(&trace_header->data_size, offset);

    ETSOC_MEM_EVICT((uint64_t *)MM_TRACE_BUFFER_BASE, offset, to_L3)

    et_trace_mm_cb_lock_release();

    return offset;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Evict_Buffer_MM_Stats
*
*   DESCRIPTION
*
*       This function Evict the MM Stats Trace buffer upto current used 
*       buffer, it also updates the trace buffer header to include buffer 
*       usage.
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
uint32_t Trace_Evict_Buffer_MM_Stats(void)
{
    struct trace_buffer_std_header_t *trace_header =
        (struct trace_buffer_std_header_t *)MM_STATS_TRACE_BUFFER_BASE;

    uint32_t offset = atomic_load_local_32(&(MM_Stats_Trace_CB.cb.offset_per_hart));

    /* Store used buffer size in buffer header. */
    atomic_store_local_32(&trace_header->data_size, offset);

    ETSOC_MEM_EVICT((uint64_t *)MM_STATS_TRACE_BUFFER_BASE, offset, to_L3)

    return offset;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Init_MM_Stats
*
*   DESCRIPTION
*
*       This function initializes Trace for dev stats in Master Minion
*       Shire.
*
*   INPUTS
*
*       trace_init_info_t    Trace init info for Master Minion shire.
*
*   OUTPUTS
*
*       int32_t           Successful status or error code.
*
***********************************************************************/
int32_t Trace_Init_MM_Stats(const struct trace_init_info_t *mm_init_info)
{
    int32_t status = STATUS_SUCCESS;
    struct trace_init_info_t hart_init_info;

    /* If init information is NULL then do default initialization. */
    if (mm_init_info == NULL)
    {
        /* Populate default Trace configurations for Master Minion. */
        hart_init_info.shire_mask = MM_SHIRE_MASK;
        hart_init_info.thread_mask = MM_DEFAULT_THREAD_MASK;
        hart_init_info.event_mask = TRACE_EVENT_STRING;
        hart_init_info.filter_mask = TRACE_EVENT_STRING_DEBUG;
        hart_init_info.threshold = MM_TRACE_BUFFER_SIZE;
    }
    /* Check if shire mask is of Master Minion and atleast one thread is enabled. */
    else if (!(mm_init_info->shire_mask & MM_SHIRE_MASK))
    {
        Log_Write(LOG_LEVEL_ERROR, "Trace_Init_MM_Stats:Invalid Init Info.\r\n");
        MM_Stats_Trace_CB.cb.enable = TRACE_DISABLE;

        status = TRACE_ERROR_INVALID_SHIRE_MASK;
    }
    else if (!(mm_init_info->thread_mask & MM_HART_MASK))
    {
        Log_Write(LOG_LEVEL_ERROR, "Trace_Init_MM_Stats:Invalid Init Info.\r\n");
        MM_Stats_Trace_CB.cb.enable = TRACE_DISABLE;

        status = TRACE_ERROR_INVALID_THREAD_MASK;
    }
    else
    {
        /* Populate given init information into per-thread Trace information structure. */
        hart_init_info.shire_mask = MM_SHIRE_MASK;
        hart_init_info.thread_mask = MM_HART_MASK;
        hart_init_info.filter_mask = mm_init_info->filter_mask;
        hart_init_info.event_mask = mm_init_info->event_mask;
        hart_init_info.threshold = mm_init_info->threshold;
    }

    if (status == STATUS_SUCCESS)
    {

        /* Common buffer for all MM Harts. */
        MM_Stats_Trace_CB.cb.size_per_hart = MM_STATS_BUFFER_SIZE;
        MM_Stats_Trace_CB.cb.base_per_hart = MM_STATS_TRACE_BUFFER_BASE;

        /* Initialize buffer lock acquire/release to NULL as they are not required for MM Stats*/
        MM_Stats_Trace_CB.cb.buffer_lock_acquire = NULL;
        MM_Stats_Trace_CB.cb.buffer_lock_release = NULL;

        /* Initialize Trace for each all Harts in Master Minion. */
        status = Trace_Init(&hart_init_info, &MM_Stats_Trace_CB.cb, TRACE_STD_HEADER);

        if (status != STATUS_SUCCESS)
        {
            status = TRACE_ERROR_MM_TRACE_CONFIG_FAILED;
        }

        /* Initialize trace buffer header. */
        struct trace_buffer_std_header_t *trace_header =
            (struct trace_buffer_std_header_t *)MM_STATS_TRACE_BUFFER_BASE;

        /* Update trace buffer header for buffer layout version and partitioning information.
           One common buffer is used by all Harts to log Tracing. */
        trace_header->magic_header = TRACE_MAGIC_HEADER;
        trace_header->type = TRACE_MM_STATS_BUFFER;
        trace_header->data_size = sizeof(struct trace_buffer_std_header_t);
        trace_header->sub_buffer_size = MM_STATS_BUFFER_SIZE;
        trace_header->sub_buffer_count = 1;
        trace_header->version.major = TRACE_VERSION_MAJOR;
        trace_header->version.minor = TRACE_VERSION_MINOR;
        trace_header->version.patch = TRACE_VERSION_PATCH;

        ETSOC_MEM_EVICT((void *)trace_header, sizeof(struct trace_buffer_std_header_t), to_L3)
    }

    /* Evict an updated control block to L2 memory. */
    ETSOC_MEM_EVICT(&MM_Stats_Trace_CB, sizeof(mm_trace_control_block_t), to_L2)

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Get_MM_Stats_CB
*
*   DESCRIPTION
*
*       This function returns the common Trace control block (CB) for Trace
*       dev stats.
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
struct trace_control_block_t *Trace_Get_MM_Stats_CB(void)
{
    return &MM_Stats_Trace_CB.cb;
}