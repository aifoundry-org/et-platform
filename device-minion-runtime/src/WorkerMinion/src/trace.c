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
#include <esperanto/device-apis/device_apis_trace_types.h>
#include <stddef.h>
#include <inttypes.h>
#include "etsoc_memory.h"
#include "trace.h"
#include "log.h"
#include "device-common/hart.h"
#include "layout.h"
#include "device-common/cacheops.h"
#include "common_trace_defs.h"

#ifndef __ASSEMBLER__
#include <assert.h>
#endif

/*
 * Compute Minion Trace control block.
 */
typedef struct cm_trace_control_block {
    struct trace_control_block_t cb;    /*!< Common Trace library control block. */
} __attribute__((aligned(64))) cm_trace_control_block_t;

/*! \def GET_CB_INDEX
    \brief Get CB index of current Hart in pre-allocated CB array.
*/
#define GET_CB_INDEX(hart_id)       ((hart_id < 2048U)? hart_id: (hart_id - 32U))

/*! \def CHECK_HART_TRACE_ENABLED
    \brief Check if Trace is enabled for given Hart.
*/
#define CHECK_HART_TRACE_ENABLED(init, id)     ((init.shire_mask & GET_SHIRE_MASK(hart_id)) && \
                                             (init.thread_mask & GET_HART_MASK(hart_id)))

/*! \def CM_BASE_HART_ID
    \brief CM Base HART ID.
*/
#define CM_BASE_HART_ID             0

/*! \def CM_TRACE_CB
    \brief A local Trace control block for a Compute Minion.
*/
#define CM_TRACE_CB                 ((cm_trace_control_block_t*)FW_CM_TRACE_CB_BASEADDR)

/************************/
/* Compile-time checks  */
/************************/
#ifndef __ASSEMBLER__

/* Ensure that CM FW trace control blocks dont cross the defined limit */
static_assert(sizeof(cm_trace_control_block_t) <= TRACE_CB_MAX_SIZE,
              "CM FW Trace control block size exceeding the size limit");

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
*       None
*
***********************************************************************/
void Trace_Init_CM(const struct trace_init_info_t *cm_init_info)
{
    struct trace_init_info_t hart_init_info;
    const uint32_t hart_id = get_hart_id();
    uint32_t hart_cb_index = GET_CB_INDEX(hart_id);

    /* If init information is NULL then do default initialization. */
    if (cm_init_info == NULL)
    {
        /* Populate default Trace configurations for Compute Minion. */
        hart_init_info.shire_mask    = CM_DEFAULT_TRACE_SHIRE_MASK;
        hart_init_info.thread_mask   = CM_DEFAULT_TRACE_THREAD_MASK;
        hart_init_info.event_mask    = TRACE_EVENT_STRING;
        hart_init_info.filter_mask   = TRACE_EVENT_STRING_WARNING;
        hart_init_info.threshold     = CM_TRACE_BUFFER_SIZE_PER_HART;
    }
    else
    {
        /* Populate given init information into per-thread Trace information structure. */
        hart_init_info.shire_mask    = cm_init_info->shire_mask;
        hart_init_info.thread_mask   = cm_init_info->thread_mask;
        hart_init_info.filter_mask   = cm_init_info->filter_mask;
        hart_init_info.event_mask    = cm_init_info->event_mask;
        hart_init_info.threshold     = cm_init_info->threshold;
    }

    /* Buffer settings for current Hart. */
    CM_TRACE_CB[hart_cb_index].cb.base_per_hart = (CM_TRACE_BUFFER_BASE +
                                        (hart_cb_index * CM_TRACE_BUFFER_SIZE_PER_HART));
    CM_TRACE_CB[hart_cb_index].cb.size_per_hart = CM_TRACE_BUFFER_SIZE_PER_HART;

    /* Initialize trace buffer header. First CM hart contains ET Trace header,
       rest of Harts contain only size of trace data in their header.*/
    if(hart_id == CM_BASE_HART_ID)
    {
        struct trace_buffer_std_header_t *trace_header =
            (struct trace_buffer_std_header_t *)CM_TRACE_BUFFER_BASE;

        trace_header->magic_header = TRACE_MAGIC_HEADER;
        trace_header->type = TRACE_CM_BUFFER;
        trace_header->data_size = 0;

        /* Set the default offset */
        CM_TRACE_CB[hart_cb_index].cb.offset_per_hart = sizeof(struct trace_buffer_std_header_t);

        /* Evict the Trace buffer standard header which is part of Base Hart's buffer.
           It is required, even if tracing is disabled ofr base because it contain buffer
           validation data as part of trace buffer standard header. */
        Trace_Evict_CM_Buffer();
    }
    else
    {
        struct trace_buffer_size_header_t *size_header =
            (struct trace_buffer_size_header_t *)CM_TRACE_CB[hart_cb_index].cb.base_per_hart;

        size_header->data_size = 0;

        /* Set the default offset */
        CM_TRACE_CB[hart_cb_index].cb.offset_per_hart = sizeof(struct trace_buffer_size_header_t);
    }

    /* Verify if the current shire and thread is enabled for tracing */
    if (CHECK_HART_TRACE_ENABLED(hart_init_info, hart_id))
    {
        if(hart_id == CM_BASE_HART_ID)
        {
            /* Initialize Trace for current Hart in Compute Minion Shire. */
            Trace_Init(&hart_init_info, &CM_TRACE_CB[hart_cb_index].cb, TRACE_STD_HEADER);
        }
        else
        {
            /* Initialize Trace for current Hart in Compute Minion Shire. */
            Trace_Init(&hart_init_info, &CM_TRACE_CB[hart_cb_index].cb, TRACE_SIZE_HEADER);
        }

        /* Evict the buffer header to L3 Cache. */
        Trace_Evict_CM_Buffer();
    }
    else
    {
        /* Disable Trace for current Hart in Compute Minion Shire. */
        CM_TRACE_CB[hart_cb_index].cb.enable = TRACE_DISABLE;
    }
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
struct trace_control_block_t* Trace_Get_CM_CB(void)
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
        if(get_hart_id() == CM_BASE_HART_ID)
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

    /* Check flag to redirect logs to Trace or UART. */
    if (control & TRACE_RT_CONTROL_LOG_TO_UART)
    {
        log_set_interface(LOG_DUMP_TO_UART);
    }
    else
    {
        log_set_interface(LOG_DUMP_TO_TRACE);
        log_write(LOG_LEVEL_DEBUG,
                "TRACE_RT_CONTROL:CM:Logs redirected to Trace buffer.\r\n");
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
    if(get_hart_id() == CM_BASE_HART_ID)
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
