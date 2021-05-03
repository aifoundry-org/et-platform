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

#include "atomic.h"
#include "cacheops.h"
#include "hart.h"
#include "layout.h"
#include "config/mm_config.h"
#include "services/log.h"
#include "services/trace.h"
#include "common_trace_defs.h"

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
    struct trace_control_block_t cb;    /*!< Common Trace library control block. */
    uint64_t cm_shire_mask;         /* Compute Minion Shire mask to fetch Trace data from CM. */
} __attribute__((aligned(64))) mm_trace_control_block_t;

/* A local Trace control block for all Master Minions. */
static mm_trace_control_block_t MM_Trace_CB = 
                {.cb = {0}, .cm_shire_mask = CM_DEFAULT_TRACE_SHIRE_MASK};

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
        hart_init_info.filter_mask   = TRACE_EVENT_STRING_WARNING;
        hart_init_info.threshold     = MM_TRACE_BUFFER_SIZE;
    }
    /* Check if shire mask is of Master Minion and atleast one thread is enabled. */
    else if ((mm_init_info->shire_mask & MM_SHIRE_MASK) && (mm_init_info->thread_mask & MM_HART_MASK))
    {
        /* Populate given init information into per-thread Trace information structure. */
        hart_init_info.shire_mask    = MM_SHIRE_MASK;
        hart_init_info.thread_mask   = mm_init_info->thread_mask & MM_HART_MASK;
        hart_init_info.filter_mask   = mm_init_info->filter_mask;
        hart_init_info.event_mask    = mm_init_info->event_mask;
        hart_init_info.threshold     = mm_init_info->threshold;
    }
    else
    {
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
        Trace_Init(&hart_init_info, &MM_Trace_CB.cb);
    }

    /* Evict an updated control block to L2 memory. */
    asm volatile("fence");
    evict(to_L2, &MM_Trace_CB, sizeof(mm_trace_control_block_t));      
    WAIT_CACHEOPS;
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
void Trace_Set_CM_Shire_Mask(uint64_t cm_mask)
{
    atomic_store_local_64(&MM_Trace_CB.cm_shire_mask, cm_mask);
}