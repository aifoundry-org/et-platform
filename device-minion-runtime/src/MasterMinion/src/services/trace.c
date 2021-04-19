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

#include "cacheops.h"
#include "hart.h"
#include "layout.h"
#include "config/mm_config.h"
#include "services/log.h"
#include "services/trace.h"

/* Local function decalration. */
inline static uint8_t get_set_bit_count(uint64_t mask, uint8_t max_bit_index);
inline static uint8_t get_lowest_set_bit(uint64_t num);

/*! \def GET_MM_BASE_BUFFER
    \brief This calculates starting address of buffer for given MM Hart information 
*/
#define GET_MM_BASE_BUFFER(buf, size, thread_mask, hart_id)                                         \
        (buf + (size * get_set_bit_count(thread_mask, get_lowest_set_bit(GET_HART_MASK(hart_id)))))

/*! \def MM_DEFAULT_THREAD_MASK
    \brief Default masks to enable Trace for Dispatcher, SQ Worker (SQW), DMA Worker : Read & Write, and Kernel Worker (KW) 
*/
#define MM_DEFAULT_THREAD_MASK   ((1UL << (DISPATCHER_BASE_HART_ID - MM_BASE_ID)) |                     \
                                  (1UL << (DMAW_BASE_HART_ID - MM_BASE_ID)) |                           \
                                  (1UL << (DMAW_BASE_HART_ID + HARTS_PER_MINION - MM_BASE_ID)) |        \
                                  (1UL << (SQW_BASE_HART_ID - MM_BASE_ID)) |                            \
                                  (1UL << (KW_BASE_HART_ID - MM_BASE_ID)))

/*
 * Master Minion Trace control block.
 */
typedef struct mm_trace_control_block {
    struct trace_control_block_t cb;    /*!< Common Trace library control block. */
} __attribute__((aligned(64))) mm_trace_control_block_t;

/* A list of local Trace control blocks for Master Minions. */
static mm_trace_control_block_t MM_Trace_CB[MM_HART_COUNT] = {0};

/************************************************************************
*
*   FUNCTION
*
*       get_lowest_set_bit
*
*   DESCRIPTION
*
*       This function gets the lowest set bit in given bit mask(number).
*
*   INPUTS
*
*       uint64_t    A bit mask in which function will find lowest set bit.
*
*   OUTPUTS
*
*       uint8_t     Index of lowest set bit starting LSB as 0th index
*
***********************************************************************/
inline static uint8_t get_lowest_set_bit(uint64_t num) 
{
    uint8_t count = 0;
    while (!(num & 1))
    {
        num >>= 1;
        ++count;
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
*       Function to get no of set bits in binary representation of passed 
*       binary number. It uses Brian Kernighan’s Algorithm 
*
*   INPUTS
*
*       uint64_t    A bit mask in which function will find set bits.
*       uint8_t     Bit index to find set bit bits lower than this index 
*                   starting with LSB as 1st index.
*
*   OUTPUTS
*
*       uint8_t     Number of set bits.
*
***********************************************************************/
inline static uint8_t get_set_bit_count(uint64_t mask, uint8_t max_bit_index)
{
    uint8_t count = 0;

    if (max_bit_index < 64)
    {
        mask = mask & (~(~0UL << max_bit_index));
    }

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
        /* Set threshold so that buffer does not spill over to next Cache line.
           It is assumed that thread will consume Cache line in which its 
           starting address i.e base_per_hart falls. */
        hart_init_info.threshold     = (uint64_t) (MM_TRACE_BUFFER_SIZE / 
                                       (get_set_bit_count(MM_DEFAULT_THREAD_MASK, MM_HART_COUNT))) - 0x40UL;
        hart_init_info.buffer        = MM_TRACE_BUFFER_BASE;
        hart_init_info.buffer_size   = MM_TRACE_BUFFER_SIZE;
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
        hart_init_info.buffer        = MM_TRACE_BUFFER_BASE;
        hart_init_info.buffer_size   = MM_TRACE_BUFFER_SIZE;
    }
    else
    {
        /* Trace init information is invalid. */
        internal_status = INVALID_TRACE_INIT_INFO;
    }
    
    if(internal_status == STATUS_SUCCESS)
    {
        /* Calculate buffer size for each HART. */
        uint64_t buf_size_per_hart = (uint64_t)(hart_init_info.buffer_size / 
                                                get_set_bit_count(hart_init_info.thread_mask, MM_HART_COUNT));

        /* Initialize Trace for each Hart in Master Minion. */
        for (uint8_t i = 0; i < MM_HART_COUNT; i++)
        {
            /* Update Trace buffer information for current Hart. 
               It is assumed that thread will consume Cache line in which its 
               starting address i.e base_per_hart falls. */
            MM_Trace_CB[i].cb.size_per_hart = buf_size_per_hart;
            MM_Trace_CB[i].cb.base_per_hart = GET_MM_BASE_BUFFER(hart_init_info.buffer, buf_size_per_hart, 
                                                           hart_init_info.thread_mask, (i+MM_BASE_ID));

            /* Initialize Trace for a Hart specified by Hart ID.  */
            Trace_Init(&hart_init_info, &MM_Trace_CB[i].cb, (uint64_t)(i + MM_BASE_ID));

            /* Evict an updated control block to L3 memory. */
            asm volatile("fence");
            evict(to_L3, &MM_Trace_CB[i], sizeof(mm_trace_control_block_t));      
            WAIT_CACHEOPS;
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       Trace_Get_MM_CB
*
*   DESCRIPTION
*
*       This function return Trace control block for given Hart ID.
*
*   INPUTS
*
*       uint64_t              Hart ID for which user needs Trace control 
*                             block.
*
*   OUTPUTS
*
*       trace_control_block_t Pointer to the Trace control block.
*
***********************************************************************/
struct trace_control_block_t* Trace_Get_MM_CB(void)
{
    return &MM_Trace_CB[get_hart_id() - MM_BASE_ID].cb;
}