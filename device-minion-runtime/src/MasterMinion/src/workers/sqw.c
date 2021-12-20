/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/***********************************************************************/
/*! \file sqw.c
    \brief A C module that implements the Submission Queue Worker's (SQW)
    public and private interfaces. The Master Minion runtime SW
    architecture enables parallel processing of commands submitted to
    Submission Queueus by dedicating one SQW per Host to Device
    Submission Queue.
    This module implements:
    1. SQW_Launch - An infinite loop that unblocks on FCC notification
    from dispatcher, pops available commands from the associated
    submission queue, decodes commands, and processes commands.
    Depending on the command, commands are either processed by the
    SQW itself, or processed by offloading some of command processing
    to other workers (i.e., KW, DMAW) in the master shire.
    2. It implements, and exposes the below listed public interfaces to
    other master shire runtime components present in the system
    to facilitate SQW management

    Public interfaces:
        SQW_Init
        SQW_Notify
        SQW_Launch
        SQW_Decrement_Command_Count
        SQW_Increment_Command_Count
        SQW_Abort_All_Pending_Commands
        SQW_Get_State
*/
/***********************************************************************/
/* comon-api, device_ops_api */
#include <esperanto/device-apis/device_apis_message_types.h>

/* mm_rt_svcs */
#include <etsoc/drivers/pmu/pmu.h>
#include <etsoc/isa/etsoc_memory.h>

/* mm_rt_helpers */
#include "error_codes.h"

/* mm specific headers */
#include "workers/sqw.h"
#include "services/log.h"
#include "services/host_iface.h"
#include "services/host_cmd_hdlr.h"
#include "services/trace.h"
#include "services/sp_iface.h"

/*! \typedef sqw_cmds_status_t
    \brief Submission Queue Worker Commands status Control Block structure
*/
typedef struct sqw_cmds_status_ {
    union {
        struct {
            int32_t cmds_count;
            uint32_t state;
        };
        uint64_t raw_u64;
    };
} sqw_cmds_status_t;

/*! \typedef sqw_cb_t
    \brief Submission Queue Worker Control Block structure
*/
typedef struct sqw_cb_ {
    sqw_cmds_status_t sqw_status[MM_SQ_COUNT];
    local_fcc_flag_t sqw_fcc_flags[MM_SQ_COUNT];
} sqw_cb_t;

/*! \var sqw_cb_t SQW_CB
    \brief Global Submission Queue Worker Control Block
    \warning Not thread safe!
*/
static sqw_cb_t SQW_CB __attribute__((aligned(64))) = { 0 };

/************************************************************************
*
*   FUNCTION
*
*       sqw_command_barrier
*
*   DESCRIPTION
*
*       Local fn helper that servers as a SQW command barrier
*
*   INPUTS
*
*       sqw_idx        Submission Queue Index
*
*   OUTPUTS
*
*       int32_t         Success or error code
*
***********************************************************************/
static inline int32_t sqw_command_barrier(uint8_t sqw_idx)
{
    sqw_cmds_status_t cmds_status;
    int32_t status = STATUS_SUCCESS;

    Log_Write(LOG_LEVEL_DEBUG, "SQW[%d]:Command Barrier\r\n", sqw_idx);

    cmds_status.raw_u64 = atomic_load_local_64(&SQW_CB.sqw_status[sqw_idx].raw_u64);
    Log_Write(LOG_LEVEL_DEBUG, "SQW[%d]:Outstanding Cmd Cnt: %d\r\n", sqw_idx,
        (uint32_t)cmds_status.cmds_count);

    /* Spin-wait until the commands count is zero */
    do
    {
        cmds_status.raw_u64 = atomic_load_local_64(&SQW_CB.sqw_status[sqw_idx].raw_u64);
        asm volatile("fence\n" ::: "memory");
    } while (((uint32_t)cmds_status.cmds_count != 0U) && (cmds_status.state == SQW_STATE_BUSY));

    /* check for barrier abort flag */
    if (cmds_status.state == SQW_STATE_ABORTED)
    {
        status = SQW_STATUS_BARRIER_ABORTED;
        Log_Write(LOG_LEVEL_ERROR, "SQW[%d]:Barrier aborted!\r\n", sqw_idx);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SQW_Init
*
*   DESCRIPTION
*
*       Initialize resources needed by SQ Workers
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
void SQW_Init(void)
{
    /* Initialize the SQ Worker sync flags */
    for (uint8_t i = 0; i < MM_SQ_COUNT; i++)
    {
        local_fcc_flag_init(&SQW_CB.sqw_fcc_flags[i]);

        atomic_store_local_64(&SQW_CB.sqw_status[i].raw_u64, 0U);
    }

    return;
}

/************************************************************************
*
*   FUNCTION
*
*       SQW_Notify
*
*   DESCRIPTION
*
*       Notify SQ Worker
*
*   INPUTS
*
*       sqw_idx     Submission Queue Worker index
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void SQW_Notify(uint8_t sqw_idx)
{
    /* Uses even Harts always */
    uint32_t minion = SQW_WORKER_0 + sqw_idx;

    Log_Write(LOG_LEVEL_DEBUG, "Notifying:SQW:minion=%d:thread=%d\r\n", minion, SQW_THREAD_ID);

    /* TODO: Future improvements: 1. To use IPIs.
    2. Improve FCC to address security concerns. */
    local_fcc_flag_notify_no_ack(&SQW_CB.sqw_fcc_flags[sqw_idx], minion, SQW_THREAD_ID);

    return;
}

/************************************************************************
*
*   FUNCTION
*
*       sqw_process_waiting_commands
*
*   DESCRIPTION
*
*       This function prefetches the VQ data in L2 SCP and processes each
*       command.
*
*   INPUTS
*
*       sqw_idx         Submission Queue Worker index
*       vq_cached       VQ cached local pointer
*       vq_shared       VQ shared SRAM pointer
*       start_cycles    Command processing start cycles
*       shared_mem_ptr  Shared memory pointer of VQ
*       vq_used_space   Total number of data bytes to process
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static inline void sqw_process_waiting_commands(uint32_t sqw_idx, vq_cb_t *vq_cached,
    vq_cb_t *vq_shared, uint64_t start_cycles, void *shared_mem_ptr, uint64_t vq_used_space)
{
    uint8_t *cmd_buff = (uint8_t *)(MM_SQ_PREFETCHED_BUFFER_BASEADDR + sqw_idx * MM_SQ_SIZE_MAX);
    const struct cmd_header_t *cmd_hdr;
    uint32_t cmd_buff_idx = 0;
    int32_t pop_ret_val;
    int32_t status;

    /* Create a shadow copy of data from SQ to L2 SCP */
    status = VQ_Prefetch_Buffer(vq_cached, vq_used_space, shared_mem_ptr, cmd_buff);

    /* Update the tail offset in VQ shared memory so that host is able to push new commands */
    Host_Iface_Optimized_SQ_Update_Tail(vq_shared, vq_cached);

    if (status != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "SQW:ERROR:VQ prefetch buffer failed:%d\r\n", status);
        return;
    }

    /* Process the commands buffer. */
    while (cmd_buff_idx < vq_used_space)
    {
        /* Process commands from L2 SCP prefetched copy */
        pop_ret_val = VQ_Process_Command(cmd_buff, vq_used_space, cmd_buff_idx);

        if (pop_ret_val > 0)
        {
            /* Set the command starting address */
            cmd_hdr = (void *)&cmd_buff[cmd_buff_idx];

            Log_Write(LOG_LEVEL_DEBUG, "SQW:Processing:SQW_IDX=%d:tag_id=%x:Popped_length:%d\r\n",
                sqw_idx, cmd_hdr->cmd_hdr.tag_id, pop_ret_val);

            /* If barrier flag is set, wait until all cmds are
            processed in the current SQ */
            if (cmd_hdr->cmd_hdr.flags & CMD_FLAGS_BARRIER_ENABLE)
            {
                TRACE_LOG_CMD_STATUS(cmd_hdr->cmd_hdr.msg_id, (uint8_t)sqw_idx,
                    cmd_hdr->cmd_hdr.tag_id, CMD_STATUS_WAIT_BARRIER);

                sqw_command_barrier((uint8_t)sqw_idx);
            }

            /* Increment the SQW command count.
            NOTE: Its Host_Command_Handler's job to ensure this
            count is decremented on the basis of the path it
            takes to process a command. */
            SQW_Increment_Command_Count((uint8_t)sqw_idx);

            /* Handle the command processing */
            status = Host_Command_Handler(&cmd_buff[cmd_buff_idx], (uint8_t)sqw_idx, start_cycles);

            if (status != STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_ERROR, "SQW:ERROR:Processing failed:%d\r\n", status);
            }

            /* Update the command buffer index */
            cmd_buff_idx += (uint32_t)pop_ret_val;
        }
        else if (pop_ret_val < 0)
        {
            Log_Write(LOG_LEVEL_ERROR, "SQW:ERROR:VQ cmd processing failed:%d\r\n", pop_ret_val);
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
*       SQW_Launch
*
*   DESCRIPTION
*
*       Launch a Submission Queue Worker on HART ID requested
*
*   INPUTS
*
*       uint32_t   HART ID to launch the dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void SQW_Launch(uint32_t hart_id, uint32_t sqw_idx)
{
    uint64_t tail_prev;
    uint64_t start_cycles = 0;
    void *shared_mem_ptr;
    circ_buff_cb_t circ_buff_cached __attribute__((aligned(64)));
    vq_cb_t vq_cached;

    /* Declare a pointer to the VQ control block that is pointing to the VQ attributes in
    global memory and the actual Circular Buffer CB in SRAM (which is shared with host) */
    vq_cb_t *vq_shared = Host_Iface_Get_VQ_Base_Addr(SQ, (uint8_t)sqw_idx);

    /* Make a copy of the VQ CB in cached DRAM to cached L1 stack variable */
    ETSOC_Memory_Read_Local_Atomic((void *)vq_shared, (void *)&vq_cached, sizeof(vq_cached));

    /* Make a copy of the Circular Buffer CB in shared SRAM to cached L1 stack variable */
    ETSOC_Memory_Read_Uncacheable(
        (void *)vq_cached.circbuff_cb, (void *)&circ_buff_cached, sizeof(circ_buff_cached));

    /* Save the shared memory pointer */
    shared_mem_ptr = (void *)vq_cached.circbuff_cb->buffer_ptr;

    /* Verify that the head pointer in cached variable and shared SRAM are 8-byte aligned addresses */
    if (!(IS_ALIGNED(&circ_buff_cached.head_offset, 8) &&
            IS_ALIGNED(&vq_cached.circbuff_cb->head_offset, 8)))
    {
        Log_Write(LOG_LEVEL_ERROR, "SQW:SQ HEAD not 64-bit aligned\r\n");
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_SQ_BUFFER_ALIGNMENT_ERROR);
    }

    /* Verify that the tail pointer in cached variable and shared SRAM are 8-byte aligned addresses */
    if (!(IS_ALIGNED(&circ_buff_cached.tail_offset, 8) &&
            IS_ALIGNED(&vq_cached.circbuff_cb->tail_offset, 8)))
    {
        Log_Write(LOG_LEVEL_ERROR, "SQW:SQ tail not 64-bit aligned\r\n");
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_SQ_BUFFER_ALIGNMENT_ERROR);
    }

    /* Update the local VQ CB to point to the cached L1 stack variable */
    vq_cached.circbuff_cb = &circ_buff_cached;

    Log_Write(LOG_LEVEL_INFO, "SQW:H%d:IDX=%d\r\n", hart_id, sqw_idx);

    while (1)
    {
        /* Update the SQW state to idle */
        atomic_store_local_32(&SQW_CB.sqw_status[sqw_idx].state, SQW_STATE_IDLE);

        Log_Write(LOG_LEVEL_DEBUG, "SQW:IDX=%d:State Idle\r\n", sqw_idx);

        /* Wait for SQ Worker notification from Dispatcher */
        local_fcc_flag_wait(&SQW_CB.sqw_fcc_flags[sqw_idx]);

        /* Update the SQW state to busy */
        atomic_store_local_32(&SQW_CB.sqw_status[sqw_idx].state, SQW_STATE_BUSY);

        Log_Write(LOG_LEVEL_DEBUG, "SQW:IDX=%d:State Busy\r\n", sqw_idx);

        /* Get current minion cycle */
        start_cycles = PMC_Get_Current_Cycles();

        Log_Write(LOG_LEVEL_DEBUG, "SQW:H%d:received FCC event!\r\n", hart_id);

        /* Get the cached tail pointer */
        tail_prev = VQ_Get_Tail_Offset(&vq_cached);

        /* Refresh the cached VQ CB - Get updated head and tail values */
        VQ_Get_Head_And_Tail(vq_shared, &vq_cached);

        /* Verify that the tail value read from memory is equal to previous tail value */
        if (tail_prev != VQ_Get_Tail_Offset(&vq_cached))
        {
            Log_Write(LOG_LEVEL_ERROR,
                "SQW:FATAL_ERROR:Tail Mismatch:Cached: %ld, Shared Memory: %ld Using cached value as fallback mechanism\r\n",
                tail_prev, VQ_Get_Tail_Offset(&vq_cached));

            SP_Iface_Report_Error(MM_RECOVERABLE, MM_SQ_PROCESSING_ERROR);

            /* TODO: Fallback mechanism: use the cached copy of SQ tail */
            vq_cached.circbuff_cb->tail_offset = tail_prev;
        }

        /* Calculate the total number of bytes available in the VQ */
        uint64_t vq_used_space = VQ_Get_Used_Space(&vq_cached, CIRCBUFF_FLAG_NO_READ);

        /* Process commands until there is no more data in VQ */
        while (vq_used_space)
        {
            /* Process the pending commands */
            sqw_process_waiting_commands(
                sqw_idx, &vq_cached, vq_shared, start_cycles, shared_mem_ptr, vq_used_space);

            /* In case of SQW aborted state, we need to read the shared copy of head and tail
            again to make sure that all the commands in VQ are processed and aborted. */
            if (atomic_load_local_32(&SQW_CB.sqw_status[sqw_idx].state) == SQW_STATE_ABORTED)
            {
                /* Refresh the cached VQ CB - Get updated head and tail values */
                VQ_Get_Head_And_Tail(vq_shared, &vq_cached);
            }

            /* Re-calculate the total number of bytes available in the cached VQ copy */
            vq_used_space = VQ_Get_Used_Space(&vq_cached, CIRCBUFF_FLAG_NO_READ);
        }
    } /* loop forever */

    /* will not return */
}

/************************************************************************
*
*   FUNCTION
*
*       SQW_Decrement_Command_Count
*
*   DESCRIPTION
*
*       Decrement outstanding command count for the given Submission
*       Queue Worker
*
*   INPUTS
*
*       sqw_idx     Submission Queue Worker index
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void SQW_Decrement_Command_Count(uint8_t sqw_idx)
{
    /* Decrement commands count being processed by current SQW */
    int32_t original_val = atomic_add_signed_local_32(&SQW_CB.sqw_status[sqw_idx].cmds_count, -1);

    if ((original_val - 1) < 0)
    {
        Log_Write(LOG_LEVEL_ERROR, "SQW[%d] Decrement:Command Counter is Negative : %d\r\n",
            sqw_idx, original_val - 1);
    }
    else
    {
        Log_Write(LOG_LEVEL_DEBUG, "SQW[%d] Decrement:Command Counter: %d\r\n", sqw_idx,
            original_val - 1);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       SQW_Increment_Command_Count
*
*   DESCRIPTION
*
*       Increment outstanding command count for the given Submission
*       Queue Worker
*
*   INPUTS
*
*       sqw_idx     Submission Queue Worker index
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void SQW_Increment_Command_Count(uint8_t sqw_idx)
{
    /* Increment commands count being processed by current SQW */
    int32_t original_val = atomic_add_signed_local_32(&SQW_CB.sqw_status[sqw_idx].cmds_count, 1);

    Log_Write(
        LOG_LEVEL_DEBUG, "SQW[%d] Increment:Command Count: %d\r\n", sqw_idx, original_val + 1);
}

/************************************************************************
*
*   FUNCTION
*
*       SQW_Abort_All_Pending_Commands
*
*   DESCRIPTION
*
*       Blocking function that aborts each in progress SQ and waits until
*       the state is back to idle.
*
*   INPUTS
*
*       sqw_idx     Submission Queue Worker index
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void SQW_Abort_All_Pending_Commands(uint8_t sqw_idx)
{
    uint32_t old_state;

    Log_Write(LOG_LEVEL_DEBUG, "SQW[%d]: Abort all pending commands\r\n", sqw_idx);

    /* Traverse SQ and check for state. If busy, set abort state
    and wait for it to be idle. This would guarantee
    that all pending commadns in SQ are aborted. */
    old_state = atomic_compare_and_exchange_local_32(
        &SQW_CB.sqw_status[sqw_idx].state, SQW_STATE_BUSY, SQW_STATE_ABORTED);

    if (old_state == SQW_STATE_BUSY)
    {
        Log_Write(LOG_LEVEL_ERROR, "SQW[%d]: Aborting all pending commands\r\n", sqw_idx);

        /* Spin-wait if the SQW state is aborted */
        do
        {
            asm volatile("fence\n" ::: "memory");
        } while (atomic_load_local_32(&SQW_CB.sqw_status[sqw_idx].state) == SQW_STATE_ABORTED);

        Log_Write(LOG_LEVEL_DEBUG, "SQW[%d]: Aborted all pending commands\r\n", sqw_idx);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       SQW_Get_State
*
*   DESCRIPTION
*
*       Function that returns the state of a submission queue worker
*
*   INPUTS
*
*       sqw_idx     Submission Queue Worker index
*
*   OUTPUTS
*
*       sqw_state_e State of the SQW
*
***********************************************************************/
sqw_state_e SQW_Get_State(uint8_t sqw_idx)
{
    return atomic_load_local_32(&SQW_CB.sqw_status[sqw_idx].state);
}
