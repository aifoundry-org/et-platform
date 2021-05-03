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
        Trace_Control_CM_RT
        Trace_Evict_CM_Buffer
*/
/***********************************************************************/

#include <stddef.h>
#include <inttypes.h>
#include "trace.h"
#include "hart.h"
#include "layout.h"
#include "cacheops.h"
#include "common_trace_defs.h"

/*! \def GET_CB_INDEX
    \brief Get CB index of current Hart in pre-allocated CB array.. 
*/
#define GET_CB_INDEX(hart_id)       ((hart_id < 2048U)? hart_id: (hart_id - 32U))

/*! \def CM_HART_COUNT
    \brief Number of Harts running CMFW. (CM Shires * Harts per Shire + MM Harts acting as Workers) 
*/
#define CM_HART_COUNT               (2080)

/*
 * Compute Minion Trace control block.
 */
typedef struct cm_trace_control_block {
    struct trace_control_block_t cb[CM_HART_COUNT];    /*!< Common Trace library control block. */
} __attribute__((aligned(64))) cm_trace_control_block_t;

/* A local Trace control block for all Compute Minions. */
static cm_trace_control_block_t CM_Trace_CB = {0};

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
    uint32_t hart_cb_index = GET_CB_INDEX(get_hart_id());

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
    CM_Trace_CB.cb[hart_cb_index].base_per_hart = (CM_TRACE_BUFFER_BASE + 
                                        (hart_cb_index * CM_TRACE_BUFFER_SIZE_PER_HART));
    CM_Trace_CB.cb[hart_cb_index].size_per_hart = CM_TRACE_BUFFER_SIZE_PER_HART;

    /* Initialize Trace for current Hart in Compute Minion Shire. */
    Trace_Init(&hart_init_info, &CM_Trace_CB.cb[hart_cb_index]);
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
    return &CM_Trace_CB.cb[GET_CB_INDEX(get_hart_id())];
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Control_CM_RT
*
*   DESCRIPTION
*
*       This function updates the control of Trace for Compute Minnion 
*       runtime. 
*
*   INPUTS
*
*       trace_enable_e    Enable / Disbale Trace.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void Trace_Control_CM_RT(enum trace_enable_e enable)
{
    CM_Trace_CB.cb[GET_CB_INDEX(get_hart_id())].enable = enable;
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
    
    /* Check if current hart any data preset in its buffer that needs to be evicted. 
       In case case of buffer overflow and reset, data eviction should be handled 
       separately. */
    if(CM_Trace_CB.cb[hart_cb_index].offset_per_hart > 0)
    {
        /* Flush the buffer from Cache to memory. */
        asm volatile("fence");
        evict(to_Mem, (uint64_t *)CM_Trace_CB.cb[hart_cb_index].base_per_hart, 
                    CM_Trace_CB.cb[hart_cb_index].offset_per_hart);      
        WAIT_CACHEOPS;
    }
}
