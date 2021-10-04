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
/***********************************************************************/
/*! \file sqw_hp.c
    \brief A C module that implements the High Priority Submission Queue
    Worker's (SQW HP) public and private interfaces.

    Public interfaces:
        SQW_HP_Init
        SQW_HP_Notify
        SQW_HP_Launch
        SQW_HP_Deccrement_Command_Count
        SQW_HP_Increment_Command_Count
*/
/***********************************************************************/
/* common-api, device_ops_api */
#include <esperanto/device-apis/device_apis_message_types.h>

/* mm_rt_svcs */
#include <etsoc/isa/etsoc_memory.h>

/* mm specific headers */
#include "workers/sqw_hp.h"
#include "services/log.h"
#include "services/host_iface.h"
#include "services/host_cmd_hdlr.h"
#include "services/trace.h"
#include "services/sp_iface.h"

/*! \typedef sqw_hp_cb_t
    \brief High Priority Submission Queue Worker Control Block structure
*/
typedef struct sqw_hp_cb_ {
    local_fcc_flag_t    sqw_fcc_flags[MM_SQ_HP_COUNT];
    int32_t             barrier_cmds_count[MM_SQ_HP_COUNT];
} sqw_hp_cb_t;

/*! \var sqw_hp_cb_t SQW_HP_CB
    \brief Global High Priority Submission Queue Worker Control Block
    \warning Not thread safe!
*/
static sqw_hp_cb_t SQW_HP_CB __attribute__((aligned(64))) = {0};

/************************************************************************
*
*   FUNCTION
*
*       sqw_hp_command_barrier
*
*   DESCRIPTION
*
*       Local fn helper that servers as a SQW HP command barrier
*
*   INPUTS
*
*       sqw_hp_idx     HP Submission Queue Index
*
*   OUTPUTS
*
*       Npne
*
***********************************************************************/
static inline void sqw_hp_command_barrier(uint8_t sqw_hp_idx)
{
    Log_Write(LOG_LEVEL_DEBUG, "SQW_HP[%d]:Command Barrier\r\n", sqw_hp_idx);

    /* Spin-wait until the commands count is zero */
    do
    {
        asm volatile("fence\n" ::: "memory");
    } while (atomic_load_local_32((uint32_t*)&SQW_HP_CB.barrier_cmds_count[sqw_hp_idx]) != 0U);
}

/************************************************************************
*
*   FUNCTION
*
*       SQW_HP_Init
*
*   DESCRIPTION
*
*       Initialize resources needed by HP SQ Workers
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
void SQW_HP_Init(void)
{
    /* Initialize the SQ HP Worker sync flags */
    for (uint8_t sqw_hp_idx = 0; sqw_hp_idx < MM_SQ_HP_COUNT; sqw_hp_idx++)
    {
        local_fcc_flag_init(&SQW_HP_CB.sqw_fcc_flags[sqw_hp_idx]);
        atomic_store_local_32((uint32_t*)&SQW_HP_CB.barrier_cmds_count[sqw_hp_idx], 0U);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       SQW_HP_Notify
*
*   DESCRIPTION
*
*       Notify HP SQ Worker
*
*   INPUTS
*
*       sqw_hp_idx     HP Submission Queue Worker index
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void SQW_HP_Notify(uint8_t sqw_hp_idx)
{
    /* Uses odd Harts always */
    uint32_t minion = SQW_HP_WORKER_0 + sqw_hp_idx;

    Log_Write(LOG_LEVEL_DEBUG, "SQW_HP:Notify:minion=%d:thread=%d\r\n",
        minion, SQW_HP_THREAD_ID);

    /* TODO: Future improvements: 1. To use IPIs.
    2. Improve FCC to address security concerns. */
    local_fcc_flag_notify_no_ack(&SQW_HP_CB.sqw_fcc_flags[sqw_hp_idx],
        minion, SQW_HP_THREAD_ID);
}

/************************************************************************
*
*   FUNCTION
*
*       sqw_hp_process_waiting_commands
*
*   DESCRIPTION
*
*       This function prefetches the VQ data in local buffer and processes
*       each command.
*
*   INPUTS
*
*       sqw_hp_idx         HP Submission Queue Worker index
*       hp_vq_cached       VQ cached local pointer
*       hp_vq_shared       VQ shared SRAM pointer
*       hp_shared_mem_ptr  Shared memory pointer of VQ
*       hp_vq_used_space   Total number of data bytes to process
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static inline void sqw_hp_process_waiting_commands(uint32_t sqw_hp_idx, vq_cb_t *hp_vq_cached,
    vq_cb_t *hp_vq_shared, void* hp_shared_mem_ptr, uint64_t hp_vq_used_space)
{
    uint8_t hp_cmd_buff[64] __attribute__((aligned(8))) = { 0 };
    const struct cmd_header_t *hp_cmd_hdr;
    int32_t processed_val;
    uint32_t cmd_buff_idx = 0;
    int8_t status;

    /* Create a shadow copy of data from HP SQ to L1 Buffer */
    status = VQ_Prefetch_Buffer(hp_vq_cached, hp_vq_used_space, hp_shared_mem_ptr, hp_cmd_buff);

    /* Update the tail offset in VQ shared memory so that host is able to push new commands */
    Host_Iface_Optimized_SQ_Update_Tail(hp_vq_shared, hp_vq_cached);

    if(status != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "SQW_HP:ERROR:VQ_Prefetch_Buffer failed:%d\r\n",
            status);
        return;
    }

    while(cmd_buff_idx < hp_vq_used_space)
    {
        /* Process commands from L1 prefetched copy */
        processed_val = VQ_Process_Command(hp_cmd_buff, hp_vq_used_space, cmd_buff_idx);

        if(processed_val > 0)
        {
            /* Set the command starting address */
            hp_cmd_hdr = (void*)&hp_cmd_buff[cmd_buff_idx];

            Log_Write(LOG_LEVEL_DEBUG,
                "SQW_HP:Processing:SQW_HP_IDX=%d:tag_id=%x:Popped_length:%d\r\n",
                sqw_hp_idx, hp_cmd_hdr->cmd_hdr.tag_id, processed_val);

            /* If barrier flag is set, wait until all cmds are
            processed in the current HP SQ */
            if(hp_cmd_hdr->cmd_hdr.flags & CMD_FLAGS_BARRIER_ENABLE)
            {
                TRACE_LOG_CMD_STATUS(hp_cmd_hdr->cmd_hdr.msg_id, (uint8_t)sqw_hp_idx,
                                     hp_cmd_hdr->cmd_hdr.tag_id, CMD_STATUS_WAIT_BARRIER);

                sqw_hp_command_barrier((uint8_t)sqw_hp_idx);
            }

            /* Increment the HP SQW command count. */
            SQW_HP_Increment_Command_Count((uint8_t)sqw_hp_idx);

            /* Process the high priority command */
            status = Host_HP_Command_Handler(&hp_cmd_buff[cmd_buff_idx],
                (uint8_t)sqw_hp_idx);

            if(status != STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_ERROR, "SQW_HP:ERROR:Processing failed:%d\r\n", status);
            }

            /* Update the command buffer index */
            cmd_buff_idx += (uint32_t)processed_val;
        }
        else if(processed_val < 0)
        {
            Log_Write(LOG_LEVEL_ERROR, "SQW_HP:ERROR:VQ cmd processing failed:%d\r\n", processed_val);
            SP_Iface_Report_Error(MM_RECOVERABLE, MM_SQ_PROCESSING_ERROR);

            /* Being pessimistic and update the command buffer index with VQ cmd header size */
            cmd_buff_idx += DEVICE_CMD_HEADER_SIZE;
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       SQW_HP_Launch
*
*   DESCRIPTION
*
*       Launch a HP Submission Queue Worker on HART ID requested
*
*   INPUTS
*
*       hart_id     HART ID to launch the HP SQ
*       sqw_hp_idx  Index of the HP SQ
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
__attribute__((noreturn)) void SQW_HP_Launch(uint32_t hart_id, uint32_t sqw_hp_idx)
{
    uint64_t hp_tail_prev;
    void* hp_shared_mem_ptr;
    circ_buff_cb_t circ_buff_cached __attribute__((aligned(64)));
    vq_cb_t hp_vq_cached;

    /* Declare a pointer to the VQ control block that is pointing to the VQ attributes in
    global memory and the actual Circular Buffer CB in SRAM (which is shared with host) */
    vq_cb_t *hp_vq_shared = Host_Iface_Get_VQ_Base_Addr(SQ_HP, (uint8_t)sqw_hp_idx);

    /* Make a copy of the VQ CB in cached DRAM to cached L1 stack variable */
    ETSOC_Memory_Read_Local_Atomic((void*)hp_vq_shared, (void*)&hp_vq_cached, sizeof(hp_vq_cached));

    /* Make a copy of the Circular Buffer CB in shared SRAM to cached L1 stack variable */
    ETSOC_Memory_Read_Uncacheable((void*)hp_vq_cached.circbuff_cb, (void*)&circ_buff_cached,
        sizeof(circ_buff_cached));

    /* Save the shared memory pointer */
    hp_shared_mem_ptr = (void*)hp_vq_cached.circbuff_cb->buffer_ptr;

    /* Verify that the head pointer in cached variable and shared SRAM are 8-byte aligned addresses */
    if (!(IS_ALIGNED(&circ_buff_cached.head_offset, 8) && IS_ALIGNED(&hp_vq_cached.circbuff_cb->head_offset, 8)))
    {
        Log_Write(LOG_LEVEL_ERROR, "SQW_HP:SQ HEAD not 64-bit aligned\r\n");
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_SQ_HP_BUFFER_ALIGNMENT_ERROR);
    }

    /* Verify that the tail pointer in cached variable and shared SRAM are 8-byte aligned addresses */
    if (!(IS_ALIGNED(&circ_buff_cached.tail_offset, 8) && IS_ALIGNED(&hp_vq_cached.circbuff_cb->tail_offset, 8)))
    {
        Log_Write(LOG_LEVEL_ERROR, "SQW_HP:SQ tail not 64-bit aligned\r\n");
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_SQ_HP_BUFFER_ALIGNMENT_ERROR);
    }

    /* Update the local VQ CB to point to the cached L1 stack variable */
    hp_vq_cached.circbuff_cb = &circ_buff_cached;

    Log_Write(LOG_LEVEL_INFO, "SQW_HP:H%d:IDX=%d\r\n", hart_id, sqw_hp_idx);

    while(1)
    {
        /* Wait for SQ Worker notification from Dispatcher */
        local_fcc_flag_wait(&SQW_HP_CB.sqw_fcc_flags[sqw_hp_idx]);

        Log_Write(LOG_LEVEL_DEBUG, "SQW_HP:H%d:received FCC event!\r\n", hart_id);

        /* Get the cached tail pointer */
        hp_tail_prev = VQ_Get_Tail_Offset(&hp_vq_cached);

        /* Refresh the cached VQ CB - Get updated head and tail values */
        VQ_Get_Head_And_Tail(hp_vq_shared, &hp_vq_cached);

        /* Verify that the tail value read from memory is equal to previous tail value */
        if(hp_tail_prev != VQ_Get_Tail_Offset(&hp_vq_cached))
        {
            Log_Write(LOG_LEVEL_ERROR,
            "SQW_HP:FATAL_ERROR:Tail Mismatch:Cached: %ld, Shared Memory: %ld Using cached value as fallback mechanism\r\n",
            hp_tail_prev, VQ_Get_Tail_Offset(&hp_vq_cached));

            SP_Iface_Report_Error(MM_RECOVERABLE, MM_SQ_HP_PROCESSING_ERROR);

            /* TODO: Fallback mechanism: use the cached copy of HP SQ tail */
            hp_vq_cached.circbuff_cb->tail_offset = hp_tail_prev;
        }

        /* Calculate the total number of bytes available in the VQ */
        uint64_t hp_vq_used_space = VQ_Get_Used_Space(&hp_vq_cached, CIRCBUFF_FLAG_NO_READ);

        /* Process commands until there is no more data in VQ */
        while(hp_vq_used_space)
        {
            /* Process the pending commands */
            sqw_hp_process_waiting_commands(sqw_hp_idx, &hp_vq_cached, hp_vq_shared,
                hp_shared_mem_ptr, hp_vq_used_space);

            /* Re-calculate the total number of bytes available in the VQ */
            hp_vq_used_space = VQ_Get_Used_Space(&hp_vq_cached, CIRCBUFF_FLAG_NO_READ);
        }
    } /* loop forever */

    /* will not return */
}

/************************************************************************
*
*   FUNCTION
*
*       SQW_HP_Decrement_Command_Count
*
*   DESCRIPTION
*
*       Decrement outstanding command count for the given HP Submission
*       Queue Worker
*
*   INPUTS
*
*       sqw_hp_idx     Submission Queue Worker index
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void SQW_HP_Decrement_Command_Count(uint8_t sqw_hp_idx)
{
    /* Decrement commands count being processed by current HP SQW */
    int32_t original_val =
        atomic_add_signed_local_32(&SQW_HP_CB.barrier_cmds_count[sqw_hp_idx], -1);

    if ((original_val - 1) < 0)
    {
        Log_Write(LOG_LEVEL_ERROR,
            "SQW_HP[%d] Decrement:Command Counter is Negative : %d\r\n",
            sqw_hp_idx, original_val -1);
    }
    else
    {
        Log_Write(LOG_LEVEL_DEBUG,
            "SQW_HP[%d] Decrement:Command Counter: %d\r\n",
            sqw_hp_idx, original_val - 1);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       SQW_HP_Increment_Command_Count
*
*   DESCRIPTION
*
*       Increment outstanding command count for the given HP Submission
*       Queue Worker
*
*   INPUTS
*
*       sqw_hp_idx     HP Submission Queue Worker index
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void SQW_HP_Increment_Command_Count(uint8_t sqw_hp_idx)
{
    /* Increment commands count being processed by current HP SQW */
    int32_t original_val =
        atomic_add_signed_local_32(&SQW_HP_CB.barrier_cmds_count[sqw_hp_idx], 1);
    (void)original_val;

    Log_Write(LOG_LEVEL_DEBUG,
        "SQW_HP[%d] Increment:Command Count: %d\r\n", sqw_hp_idx, original_val + 1);
}
