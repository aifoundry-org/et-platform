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
    \brief A C module that implements the Trace services for Compute Minions

    Public interfaces:
        Trace_Init_CM
        Trace_Get_CM_CB
        Trace_RT_Control_CM
        Trace_Evict_CM_Buffer
*/
/***********************************************************************/

#include <esperanto/device-apis/operations-api/device_ops_api_spec.h>
#include <stddef.h>
#include <inttypes.h>
#include <etsoc/drivers/pmu/pmu.h>
#include <etsoc/isa/etsoc_memory.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/cacheops.h>
#include <system/layout.h>

#include "error_codes.h"
#include "cm_mm_defines.h"
#include "common_utils.h"
#include "cm_to_mm_iface.h"
#include "cm_mm_defines.h"

#include <common/printf.h>

#define ET_TRACE_GET_HART_ID()                        get_hart_id()
#define ET_TRACE_GET_TIMESTAMP()                      PMC_Get_Current_Cycles()
#define ET_TRACE_GET_HPM_COUNTER(id)                  pmu_core_counter_read_unpriv(id)
#define ET_TRACE_VSNPRINTF(buffer, count, format, va) vsnprintf(buffer, count, format, va)
#define ET_TRACE_STRING_MAX_SIZE                      128

#define ET_TRACE_ENCODER_IMPL
#include "trace.h"

/* The log header should be included after the trace encoder is initialized. */
#include "log.h"

#ifndef __ASSEMBLER__
#include <assert.h>
#endif

/*
 * Compute Minion Trace control block.
 */
typedef struct trace_smode_control_block {
    struct trace_control_block_t cb; /*!< Common Trace library control block. */
} __attribute__((aligned(64))) trace_smode_control_block_t;

/*
 * Compute Minion UMode Trace control block.
 */
typedef struct trace_umode_control_block {
    struct trace_control_block_t cb; /*!< Common Trace library control block. */
} __attribute__((aligned(64))) trace_umode_control_block_t;

/*! \def GET_CB_INDEX
    \brief Get CB index of current Hart in pre-allocated CB array.
*/
#define GET_CB_INDEX(hart_id) ((hart_id < 2048U) ? hart_id : (hart_id - 32U))

/*! \def CHECK_HART_TRACE_ENABLED
    \brief Check if Trace is enabled for given Hart.
*/
#define CHECK_HART_TRACE_ENABLED(init, id) \
    (((init)->shire_mask & TRACE_SHIRE_MASK(id)) && ((init)->thread_mask & TRACE_HART_MASK(id)))

/*! \def CM_BASE_HART_ID
    \brief CM Base HART ID.
*/
#define CM_BASE_HART_ID 0
#define MASK_64BIT      (~0x0UL)

/*! \def CM_TRACE_CB
    \brief A local Trace control block for a Compute Minion.
*/
#define CM_TRACE_CB ((trace_smode_control_block_t *)CM_SMODE_TRACE_CB_BASEADDR)

/*! \def CM_UMODE_TRACE_CB
    \brief A local Trace control block for a Compute Minion.
*/
#define CM_UMODE_TRACE_CB ((trace_umode_control_block_t *)CM_UMODE_TRACE_CB_BASEADDR)

/************************/
/* Compile-time checks  */
/************************/
#ifndef __ASSEMBLER__

/* Ensure that CM FW trace control blocks dont cross the defined limit */
static_assert(sizeof(trace_smode_control_block_t) <= TRACE_CB_MAX_SIZE,
    "CM FW Trace control block size exceeding the size limit");

/* Ensure that CM UMode trace control blocks dont cross the defined limit */
static_assert(sizeof(trace_umode_control_block_t) <= TRACE_CB_MAX_SIZE,
    "CM UMode Trace control block size exceeding the size limit");

#endif /* __ASSEMBLER__ */

/* Static functions */
static inline void et_trace_threshold_notify(const struct trace_control_block_t *cb)
{
    const struct trace_buffer_std_header_t *trace_header =
        (const struct trace_buffer_std_header_t *)(uintptr_t)ET_TRACE_READ_U64(cb->base_per_hart);

    if (trace_header->type == TRACE_CM_BUFFER)
    {
        cm_to_mm_message_fw_trace_buffer_full_t message = {
            .header.id = CM_TO_MM_MESSAGE_ID_FW_TRACE_BUFFER_FULL,
            .data_size = cb->threshold,
            .buffer_type = (uint8_t)trace_header->type
        };

        /* Send message to dispatcher (Master shire Hart 0) - ignore error in case of failure */
        CM_To_MM_Iface_Unicast_Send(CM_MM_MASTER_HART_DISPATCHER_IDX,
            CM_MM_MASTER_HART_UNICAST_BUFF_IDX, (cm_iface_message_t *)&message);
    }
    else if (trace_header->type == TRACE_CM_UMODE_BUFFER)
    {
        /* From U-mode context, do a syscall to generate
        event to CM FW which will gnerate event to MM FW */
    }
}

/* Function to get the Hart ID of the first enabled hart in the given shire and thread mask */
static inline uint32_t trace_get_first_umode_hart_id(uint64_t shire_mask, uint64_t thread_mask)
{
    /* Check if only the master shire is enabled */
    if (shire_mask == MM_SHIRE_MASK)
    {
        return (MASTER_SHIRE * HARTS_PER_SHIRE) +
               (get_lsb_set_pos(thread_mask & CW_IN_MM_SHIRE) - 1U);
    }
    else
    {
        return ((get_lsb_set_pos(shire_mask) - 1U) * HARTS_PER_SHIRE) +
               (get_lsb_set_pos(thread_mask) - 1U);
    }
}

/* Get the Hart index of given Hart among all enabled Harts given Shire and Thread mask. */
static uint32_t trace_get_umode_buffer_idx(
    uint64_t shire_mask, uint64_t thread_mask, uint64_t hart_id)
{
    uint64_t curr_hart_shire_mask = TRACE_SHIRE_MASK(hart_id);
    uint64_t lower_thread = 0;
    uint32_t enabled_hart_index = 0;

    /* Get the bit index of the current hart's shire */
    uint32_t lsb = get_lsb_set_pos(curr_hart_shire_mask);
    lsb = lsb > 0 ? lsb - 1 : 0;

    /* Get number of shires enabled before current shire */
    uint64_t lower_shire = shire_mask & ~(MASK_64BIT << lsb);

    /* Do not enter this condition if it is the first enabled shire for tracing */
    if (get_set_bit_count(lower_shire) != 0)
    {
        /* Move the index to the start of the current shire */
        enabled_hart_index = get_set_bit_count(lower_shire) * get_set_bit_count(thread_mask);
    }

    /* Get the bit index of the current hart in the shire */
    lsb = get_lsb_set_pos(TRACE_HART_MASK(hart_id));
    lsb = lsb > 0 ? lsb - 1 : 0;

    /* Get number of harts enabled before current hart.
    Check if the current hart ID is from master shire */
    if (curr_hart_shire_mask == MM_SHIRE_MASK)
    {
        /* Drop the lower threads since those are for master shire */
        lower_thread = thread_mask & ~(MASK_64BIT << lsb);
        lower_thread >>= MM_HART_COUNT;
    }
    else
    {
        lower_thread = thread_mask & ~(MASK_64BIT << lsb);
    }

    enabled_hart_index += get_set_bit_count(lower_thread);

    return enabled_hart_index;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Init_CM
*
*   DESCRIPTION
*
*       This function initializes Trace for a single Hart in CM Shires.
*       All CM Harts must call this function to Enable Trace.
*
*   INPUTS
*
*       trace_init_info_t    Trace init info for Compute Minion shire.
*                            NULL for default configs.
*
*   OUTPUTS
*
*       int32_t           Successful status or error code.
*
***********************************************************************/
int32_t Trace_Init_CM(const struct trace_init_info_t *cm_init_info)
{
    int32_t status = STATUS_SUCCESS;
    struct trace_init_info_t hart_init_info;
    const uint32_t hart_id = get_hart_id();
    uint32_t hart_cb_index = GET_CB_INDEX(hart_id);

    /* If init information is NULL then do default initialization. */
    if (cm_init_info == NULL)
    {
        /* Populate default Trace configurations for Compute Minion. */
        hart_init_info.shire_mask = CM_DEFAULT_TRACE_SHIRE_MASK;
        hart_init_info.thread_mask = CM_DEFAULT_TRACE_THREAD_MASK;
        hart_init_info.event_mask = TRACE_EVENT_STRING;
        /* Set Trace default log level to Debug. This log level is independent from Log component. */
        hart_init_info.filter_mask = TRACE_EVENT_STRING_DEBUG;
        hart_init_info.threshold = CM_SMODE_TRACE_BUFFER_SIZE_PER_HART;
    }
    else
    {
        /* Populate given init information into per-thread Trace information structure. */
        hart_init_info.shire_mask = cm_init_info->shire_mask;
        hart_init_info.thread_mask = cm_init_info->thread_mask;
        hart_init_info.filter_mask = cm_init_info->filter_mask;
        hart_init_info.event_mask = cm_init_info->event_mask;
        hart_init_info.threshold = cm_init_info->threshold;
    }

    /* Buffer settings for current Hart. */
    CM_TRACE_CB[hart_cb_index].cb.base_per_hart =
        (CM_SMODE_TRACE_BUFFER_BASE + (hart_cb_index * CM_SMODE_TRACE_BUFFER_SIZE_PER_HART));
    CM_TRACE_CB[hart_cb_index].cb.size_per_hart = CM_SMODE_TRACE_BUFFER_SIZE_PER_HART;

    /* Initialize buffer lock acquire/release to NULL as they are not required for CM. */
    CM_TRACE_CB[hart_cb_index].cb.buffer_lock_acquire = NULL;
    CM_TRACE_CB[hart_cb_index].cb.buffer_lock_release = NULL;

    /* Register the trace buffer threshold notification */
    CM_TRACE_CB[hart_cb_index].cb.threshold_notify = et_trace_threshold_notify;

    /* Initialize trace buffer header. First CM hart contains ET Trace header,
       rest of Harts contain only size of trace data in their header.*/
    if (hart_id == CM_BASE_HART_ID)
    {
        struct trace_buffer_std_header_t *trace_header =
            (struct trace_buffer_std_header_t *)CM_SMODE_TRACE_BUFFER_BASE;

        trace_header->magic_header = TRACE_MAGIC_HEADER;
        trace_header->type = TRACE_CM_BUFFER;
        trace_header->data_size = sizeof(struct trace_buffer_size_header_t);

        /* Update trace buffer header for buffer partitioning information. Update number of buffers
        based on Trace enabled Harts. Exclude Master Shire from this calculation. */
        trace_header->sub_buffer_count = (uint16_t)(
            ((get_msb_set_pos(hart_init_info.shire_mask & CM_SHIRE_MASK) - 1U) * HARTS_PER_SHIRE) +
            get_msb_set_pos(hart_init_info.thread_mask));

        /* Check if Master Shire Compute Minions tracing needs to be initialized */
        if (hart_init_info.shire_mask & MM_SHIRE_MASK)
        {
            trace_header->sub_buffer_count =
                (uint16_t)(trace_header->sub_buffer_count + MASTER_SHIRE_COMPUTE_HARTS);
        }

        /* Buffer is divided equally among all CM Harts, with fixed size per Hart. */
        trace_header->sub_buffer_size = CM_SMODE_TRACE_BUFFER_SIZE_PER_HART;

        /* populate Trace layout version in Header. */
        trace_header->version.major = TRACE_VERSION_MAJOR;
        trace_header->version.minor = TRACE_VERSION_MINOR;
        trace_header->version.patch = TRACE_VERSION_PATCH;

        /* Set the default offset */
        CM_TRACE_CB[hart_cb_index].cb.offset_per_hart = sizeof(struct trace_buffer_std_header_t);

        /* Initialize Trace for current Hart in Compute Minion Shire. */
        status = Trace_Init(&hart_init_info, &CM_TRACE_CB[hart_cb_index].cb, TRACE_STD_HEADER);
        if (status != STATUS_SUCCESS)
        {
            status = TRACE_ERROR_CM_TRACE_CONFIG_FAILED;
        }
    }
    else
    {
        struct trace_buffer_size_header_t *size_header =
            (struct trace_buffer_size_header_t *)CM_TRACE_CB[hart_cb_index].cb.base_per_hart;

        size_header->data_size = sizeof(struct trace_buffer_size_header_t);

        /* Set the default offset */
        CM_TRACE_CB[hart_cb_index].cb.offset_per_hart = sizeof(struct trace_buffer_size_header_t);

        /* Initialize Trace for current Hart in Compute Minion Shire. */
        status = Trace_Init(&hart_init_info, &CM_TRACE_CB[hart_cb_index].cb, TRACE_SIZE_HEADER);
        if (status != STATUS_SUCCESS)
        {
            status = TRACE_ERROR_CM_TRACE_CONFIG_FAILED;
        }
    }

    /* Verify if the current shire and thread is enabled for tracing */
    if (!CHECK_HART_TRACE_ENABLED(&hart_init_info, hart_id))
    {
        /* Disable Trace for current Hart in Compute Minion Shire. */
        CM_TRACE_CB[hart_cb_index].cb.enable = TRACE_DISABLE;
    }

    /* Evict the buffer header to L3 Cache. */
    Trace_Evict_CM_Buffer();

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Configure_CM
*
*   DESCRIPTION
*
*       This function configures the Trace.
*       NOTE:Trace must be initialized using Trace_Init_CM() before this
*       function.
*
*   INPUTS
*
*       trace_config_info_t    Trace config info.
*
*   OUTPUTS
*
*       int32_t           Successful status or error code.
*
***********************************************************************/
int32_t Trace_Configure_CM(const struct trace_config_info_t *cm_config_info)
{
    int32_t status = TRACE_ERROR_INVALID_TRACE_CONFIG_INFO;
    const uint32_t hart_cb_index = GET_CB_INDEX(get_hart_id());

    /* Check if init information pointer is NULL. */
    if (cm_config_info != NULL)
    {
        status = Trace_Config(cm_config_info, &CM_TRACE_CB[hart_cb_index].cb);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Get_CM_CB
*
*   DESCRIPTION
*
*       This function returns the Trace control block (CB) of
*       the Worker Hart which is calling this function.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       trace_control_block_t   Pointer to the Trace control block.
*
***********************************************************************/
struct trace_control_block_t *Trace_Get_CM_CB(void)
{
    return &CM_TRACE_CB[GET_CB_INDEX(get_hart_id())].cb;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_RT_Control_CM
*
*   DESCRIPTION
*
*       This function updates the control of Trace for Compute Minnion
*       runtime.
*
*   INPUTS
*
*       uint32_t    Bit encoded trace control flags.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void Trace_RT_Control_CM(uint32_t control)
{
    /* Check flag to reset Trace buffer. */
    if (control & TRACE_RT_CONTROL_RESET_TRACEBUF)
    {
        if (get_hart_id() == CM_BASE_HART_ID)
        {
            CM_TRACE_CB[GET_CB_INDEX(get_hart_id())].cb.offset_per_hart =
                sizeof(struct trace_buffer_std_header_t);
        }
        else
        {
            CM_TRACE_CB[GET_CB_INDEX(get_hart_id())].cb.offset_per_hart =
                sizeof(struct trace_buffer_size_header_t);
        }
    }

    /* Check flag to Enable/Disable Trace. */
    if (control & TRACE_RT_CONTROL_ENABLE_TRACE)
    {
        Trace_Set_Enable_CM(TRACE_ENABLE);
    }
    else
    {
        Trace_Set_Enable_CM(TRACE_DISABLE);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Enable_CM
*
*   DESCRIPTION
*
*       This function enables/disables Trace for Compute Minnion
*       If its Trace disable command then it will also evict the Trace buffer
*       to L3 Cache.
*
*   INPUTS
*
*       trace_enable_e  Enum to Enable/Disable Trace.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void Trace_Set_Enable_CM(trace_enable_e control)
{
    CM_TRACE_CB[GET_CB_INDEX(get_hart_id())].cb.enable = control;
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Evict_CM_Buffer
*
*   DESCRIPTION
*
*       This function evicts the Trace buffer of caller Worker Hart.
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
void Trace_Evict_CM_Buffer(void)
{
    uint32_t hart_cb_index = GET_CB_INDEX(get_hart_id());

    /* Populate data size in trace buffer header. */
    if (get_hart_id() == CM_BASE_HART_ID)
    {
        struct trace_buffer_std_header_t *trace_header =
            (struct trace_buffer_std_header_t *)CM_TRACE_CB[hart_cb_index].cb.base_per_hart;

        trace_header->data_size = CM_TRACE_CB[hart_cb_index].cb.offset_per_hart;
    }
    else
    {
        struct trace_buffer_size_header_t *size_header =
            (struct trace_buffer_size_header_t *)CM_TRACE_CB[hart_cb_index].cb.base_per_hart;

        size_header->data_size = CM_TRACE_CB[hart_cb_index].cb.offset_per_hart;
    }

    /* Flush the buffer from Cache to Memory. */
    ETSOC_MEM_EVICT((uint64_t *)CM_TRACE_CB[hart_cb_index].cb.base_per_hart,
        CM_TRACE_CB[hart_cb_index].cb.offset_per_hart, to_L3)
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Init_UMode
*
*   DESCRIPTION
*
*       This function initializes Trace for a single Compute Hart.
*       All Harts can call this function to Enable/Dsiable its Trace.
*       Shire and Thread decides if Trace need to be enabled or disabled
*       for caller Hart.
*
*   INPUTS
*
*       trace_init_info_t       Trace init info for Compute Minion shire.
*                               NULL value initialize the Trace in default
*                               config. By default UMode Trace is disabled.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Trace_Init_UMode(const struct trace_init_info_t *init_info)
{
    const uint32_t hart_id = get_hart_id();
    struct trace_control_block_t *cb = &CM_UMODE_TRACE_CB[GET_CB_INDEX(hart_id)].cb;

    /* Passing null in place of init info disable trace */
    if (init_info == NULL)
    {
        /* Disable Trace for current Hart in Compute Minion Shire. */
        cb->enable = TRACE_DISABLE;
        return;
    }

    /* Since MM evicted the config data to L3, invalidate the lines for the current hart */
    ETSOC_MEM_EVICT((void *)(uintptr_t)init_info, sizeof(struct trace_init_info_t), to_L3)

    /* Verify if the current shire and thread is enabled for tracing
    and the buffer size is valid too */
    if (!CHECK_HART_TRACE_ENABLED(init_info, hart_id) || (init_info->buffer_size == 0))
    {
        /* Disable Trace for current Hart in Compute Minion Shire. */
        cb->enable = TRACE_DISABLE;
        return;
    }

    uint32_t enabled_harts = get_enabled_umode_harts(init_info->shire_mask, init_info->thread_mask);
    if (enabled_harts > 0)
    {
        /* Calculate size per Hart by diving total buffer equally among enabled Harts.*/
        cb->size_per_hart = init_info->buffer_size / enabled_harts;

        /* Buffer size should be cache line aligned. */
        if (cb->size_per_hart % CACHE_LINE_SIZE != 0)
        {
            cb->size_per_hart = cb->size_per_hart - (cb->size_per_hart % CACHE_LINE_SIZE);
        }
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "Trace_Init_UMode: No trace harts enabled.\r\n");
        cb->enable = TRACE_DISABLE;
        return;
    }

    /* Initialize buffer lock acquire/release to NULL as they are not required for CM. */
    cb->buffer_lock_acquire = NULL;
    cb->buffer_lock_release = NULL;

    /* TODO: Need to send notification for U-mode kernels? */
    cb->threshold_notify = NULL;

    /* Buffer settings for current Hart. */
    cb->base_per_hart = init_info->buffer + (trace_get_umode_buffer_idx(init_info->shire_mask,
                                                 init_info->thread_mask, hart_id) *
                                                cb->size_per_hart);

    /* Verify the base address and size for out of bounds */
    if (cb->base_per_hart + cb->size_per_hart > init_info->buffer + init_info->buffer_size)
    {
        Log_Write(LOG_LEVEL_ERROR,
            "Trace_Init_UMode:Trace buffer going out of bounds:base_per_hart:0x%lx:size_per_hart:0x%x\r\n",
            cb->base_per_hart, cb->size_per_hart);
    }

    /* Initialize Trace and set up buffer header. Among all enabled Harts, the first
    Compute hart's buffer has trace standard header, rest of Harts has trace size header. */
    if (hart_id == trace_get_first_umode_hart_id(init_info->shire_mask, init_info->thread_mask))
    {
        struct trace_buffer_std_header_t *trace_header =
            (struct trace_buffer_std_header_t *)cb->base_per_hart;

        trace_header->magic_header = TRACE_MAGIC_HEADER;
        trace_header->type = TRACE_CM_UMODE_BUFFER;
        trace_header->data_size = 0;

        /* Update trace buffer header for buffer partitioning information.
        Buffer is divided equally among all Trace enabled Harts. */
        trace_header->sub_buffer_count =
            (uint16_t)get_enabled_umode_harts(init_info->shire_mask, init_info->thread_mask);
        trace_header->sub_buffer_size = cb->size_per_hart;

        /* populate Trace layout version in Header. */
        trace_header->version.major = TRACE_VERSION_MAJOR;
        trace_header->version.minor = TRACE_VERSION_MINOR;
        trace_header->version.patch = TRACE_VERSION_PATCH;

        /* Set the default offset */
        cb->offset_per_hart = sizeof(struct trace_buffer_std_header_t);

        /* Initialize Trace for current Hart in Compute Minion Shire. */
        Trace_Init(init_info, cb, TRACE_STD_HEADER);
    }
    else
    {
        struct trace_buffer_size_header_t *size_header =
            (struct trace_buffer_size_header_t *)cb->base_per_hart;

        size_header->data_size = 0;

        /* Set the default offset */
        cb->offset_per_hart = sizeof(struct trace_buffer_size_header_t);

        /* Initialize Trace for current Hart in Compute Minion Shire. */
        Trace_Init(init_info, cb, TRACE_SIZE_HEADER);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Update_UMode_Buffer_Header
*
*   DESCRIPTION
*
*       This function updates Trace header to update valid data size.
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
void Trace_Update_UMode_Buffer_Header(void)
{
    const struct trace_control_block_t *cb = &CM_UMODE_TRACE_CB[GET_CB_INDEX(get_hart_id())].cb;

    if (cb->enable == TRACE_ENABLE)
    {
        if (cb->header == TRACE_STD_HEADER)
        {
            struct trace_buffer_std_header_t *trace_header =
                (struct trace_buffer_std_header_t *)cb->base_per_hart;

            trace_header->data_size = cb->offset_per_hart;
        }
        else
        {
            struct trace_buffer_size_header_t *size_header =
                (struct trace_buffer_size_header_t *)cb->base_per_hart;

            size_header->data_size = cb->offset_per_hart;
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Evict_UMode_Buffer
*
*   DESCRIPTION
*
*       This function evicts the Trace buffer of caller Worker Hart.
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
void Trace_Evict_UMode_Buffer(void)
{
    const struct trace_control_block_t *cb = &CM_UMODE_TRACE_CB[GET_CB_INDEX(get_hart_id())].cb;

    if (cb->enable == TRACE_ENABLE)
    {
        /* Updated buffer header. */
        Trace_Update_UMode_Buffer_Header();

        /* Flush the buffer from Cache to Memory. */
        ETSOC_MEM_EVICT((uint64_t *)cb->base_per_hart, cb->offset_per_hart, to_L3)
    }
}
