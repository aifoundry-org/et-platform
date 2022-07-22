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

#include "log.h"
#include "error_codes.h"
#include "cm_mm_defines.h"

#define ET_TRACE_GET_HART_ID()       get_hart_id()
#define ET_TRACE_GET_TIMESTAMP()     PMC_Get_Current_Cycles()
#define ET_TRACE_GET_HPM_COUNTER(id) pmu_core_counter_read_unpriv(id)
#define ET_TRACE_STRING_MAX_SIZE     128

#define ET_TRACE_ENCODER_IMPL
#include "trace.h"

/************************/
/* Compile-time checks  */
/************************/
#ifndef __ASSEMBLER__

/* Ensure that Max trace size is in sync.
   NOTE: This will be rmoved as result of SW-13550. */
static_assert(ET_TRACE_STRING_MAX_SIZE == TRACE_STRING_MAX_SIZE_CM,
    "MM Trace Max string size does not match with Trace encoder");

#endif /* __ASSEMBLER__ */

#ifndef __ASSEMBLER__
#include <assert.h>
#endif

/* Helper functions for bit operations. */
static uint32_t get_set_bit_count(uint64_t mask);
static uint32_t get_lsb_set_pos(uint64_t value);
static uint32_t get_msb_set_pos(uint64_t value);
static uint32_t get_index_among_enabled_harts(uint64_t shire, uint64_t thread, uint64_t hart_id);

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

/*! \def GET_TRACE_ENABLED_HART_COUNT
    \brief get number of Harts for which Trace is enabled.
*/
#define GET_TRACE_ENABLED_HART_COUNT(shire, thread) \
    (get_set_bit_count(shire) * get_set_bit_count(thread))

/*! \def GET_FIRST_TRACE_ENABLED_HART_ID
    \brief Get the lowest (first) Hart ID for which Trace is enabled.
*/
#define GET_FIRST_TRACE_ENABLED_HART_ID(shire, thread) \
    (((get_lsb_set_pos(shire) - 1U) * HARTS_PER_SHIRE) + (get_lsb_set_pos(thread) - 1U))

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

    /* Verify if the current shire and thread is enabled for tracing */
    if ((init_info == NULL) || (!CHECK_HART_TRACE_ENABLED(init_info, hart_id)) ||
        (init_info->buffer_size == 0))
    {
        /* Disable Trace for current Hart in Compute Minion Shire. */
        cb->enable = TRACE_DISABLE;
        return;
    }

    /* Initialize buffer lock acquire/release to NULL as they are not required for CM. */
    cb->buffer_lock_acquire = NULL;
    cb->buffer_lock_release = NULL;

    /* Calculate size per Hart by diving total buffer equally among enabled Harts.*/
    cb->size_per_hart =
        (init_info->buffer_size /
            GET_TRACE_ENABLED_HART_COUNT(init_info->shire_mask, init_info->thread_mask));

    /* Buffer size should be cache line aligned. */
    if (cb->size_per_hart % CACHE_LINE_SIZE != 0)
    {
        cb->size_per_hart = cb->size_per_hart - (cb->size_per_hart % CACHE_LINE_SIZE);
        /* TODO: Since per Hart buffer size is Cache-line aligned. If we have free cache lines left due to enforcing said alignment.
            Then append those free cache lines in last Hart's buffer. */
    }

    /* Buffer settings for current Hart. */
    cb->base_per_hart = (init_info->buffer + (get_index_among_enabled_harts(init_info->shire_mask,
                                                  init_info->thread_mask, hart_id) *
                                                 cb->size_per_hart));

    /* Initialize Trace and set up buffer header. Among all enabled Harts, the first Compute hart's Buffer has
      Trace standard header, rest of Harts has Trace size header.*/
    if (hart_id == GET_FIRST_TRACE_ENABLED_HART_ID(init_info->shire_mask, init_info->thread_mask))
    {
        struct trace_buffer_std_header_t *trace_header =
            (struct trace_buffer_std_header_t *)cb->base_per_hart;

        trace_header->magic_header = TRACE_MAGIC_HEADER;
        trace_header->type = TRACE_CM_UMODE_BUFFER;
        trace_header->data_size = 0;

        /* Update trace buffer header for buffer partitioning information. Buffer is divided equally among all Trace enabled Harts. */
        trace_header->sub_buffer_count =
            (uint16_t)GET_TRACE_ENABLED_HART_COUNT(init_info->shire_mask, init_info->thread_mask);
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

    /* Updated buffer header. */
    Trace_Update_UMode_Buffer_Header();

    /* Flush the buffer from Cache to Memory. */
    ETSOC_MEM_EVICT((uint64_t *)cb->base_per_hart, cb->offset_per_hart, to_L3)
}

/************************************************************************
*
*   FUNCTION
*
*       get_index_among_enabled_harts
*
*   DESCRIPTION
*
*       Get the Hart index (low to high Hart IDs) of given Hart among all
*       enabled Harts given Shire and Thread mask.
*
*   INPUTS
*
*       shire       Shire Mask.
*       thread      Thread Mask.
*       hart_id     Current Hart ID.
*
*   OUTPUTS
*
*       uint32_t    Index of given Hart ID among all enabled Harts.
*
***********************************************************************/
static uint32_t get_index_among_enabled_harts(uint64_t shire, uint64_t thread, uint64_t hart_id)
{
    uint32_t enabled_hart_index = 0;
    /* Get number of Harts enabled in lower shires.*/
    uint64_t lower_shire = shire & (~((MASK_64BIT) << get_lsb_set_pos(TRACE_SHIRE_MASK(hart_id))));

    if (!((TRACE_SHIRE_MASK(hart_id) & 1) || (get_set_bit_count(lower_shire)) == 1))
    {
        enabled_hart_index = (get_set_bit_count(lower_shire) - 1) * get_set_bit_count(thread);
    }

    /* Get number of lower Harts enabled in current shire.*/
    uint64_t lower_thread = thread & (~((MASK_64BIT) << get_lsb_set_pos(TRACE_HART_MASK(hart_id))));

    enabled_hart_index += (get_set_bit_count(lower_thread) - 1);

    return enabled_hart_index;
}

/************************************************************************
*
*   FUNCTION
*
*       get_set_bit_count
*
*   DESCRIPTION
*
*       Count number of set bits in given bit mask
*
*   INPUTS
*
*       mask       Bit mask.
*
*   OUTPUTS
*
*       uint32_t   Number of set bit in mask
*
***********************************************************************/
static uint32_t get_set_bit_count(uint64_t mask)
{
    uint32_t count = 0;
    while (mask)
    {
        mask &= (mask - 1);
        count++;
    }
    return count;
}

/************************************************************************
*
*   FUNCTION
*
*       get_set_bit_count
*
*   DESCRIPTION
*
*       Get the first least significat bit which is set in given mask.
*
*   INPUTS
*
*       mask       Bit mask.
*
*   OUTPUTS
*
*       uint32_t   Bit position of first set LSB. Zero means no bit was set.
*
***********************************************************************/
static uint32_t get_lsb_set_pos(uint64_t value)
{
    uint32_t pos = 0;

    if (value != 0)
    {
        pos = 1;
        while (!(value & 1))
        {
            value >>= 1;
            ++pos;
        }
    }
    return pos;
}

/************************************************************************
*
*   FUNCTION
*
*       get_msb_set_pos
*
*   DESCRIPTION
*
*       Get the first most significat bit which is set in given mask.
*
*   INPUTS
*
*       mask       Bit mask.
*
*   OUTPUTS
*
*       uint32_t   Bit position of first set MSB. Zero means no bit was set.
*
***********************************************************************/
static uint32_t get_msb_set_pos(uint64_t value)
{
    uint32_t msb_pos = 0;

    while (value != 0)
    {
        value = value / 2;
        msb_pos++;
    }

    return msb_pos;
}
