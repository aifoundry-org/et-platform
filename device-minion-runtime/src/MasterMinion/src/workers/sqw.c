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
    This module implements;
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
*/
/***********************************************************************/
#include "workers/sqw.h"
#include "services/log.h"
#include "services/host_iface.h"
#include "services/host_cmd_hdlr.h"
#include <esperanto/device-apis/device_apis_message_types.h>
#include "pmu.h"
#include "etsoc_memory.h"

/*! \typedef sqw_cb_t
    \brief Submission Queue Worker Control Block structure
*/
typedef struct sqw_cb_ {
    int32_t             sqw_cmd_count[MM_SQ_COUNT];
    local_fcc_flag_t    sqw_fcc_flags[MM_SQ_COUNT];
} sqw_cb_t;

/*! \var sqw_cb_t SQW_CB
    \brief Global Submission Queue Worker Control Block
    \warning Not thread safe!
*/
static sqw_cb_t SQW_CB __attribute__((aligned(64))) = {0};

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
*       sqw_idx     Submission Queue Index
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static inline void sqw_command_barrier(uint8_t sqw_idx)
{
    Log_Write(LOG_LEVEL_DEBUG, "SQW:Command Barrier\r\n");

    /* Spin-wait until the commands count is zero */
    while (atomic_load_local_32(
        (uint32_t*)&SQW_CB.sqw_cmd_count[sqw_idx]) != 0U) {
        asm volatile("fence\n" ::: "memory");
    }
    asm volatile("fence\n" ::: "memory");

    /* TODO: Add timeout and send asynchronous event back to host to
       indicate barrier timeout. Should we drop the barrier command
       from SQ? */
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

        atomic_store_local_32((uint32_t*)&SQW_CB.sqw_cmd_count[i], 0U);
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
    uint32_t minion = (uint32_t)SQW_WORKER_0 + (sqw_idx / 2);
    uint32_t thread = sqw_idx % 2;

    Log_Write(LOG_LEVEL_DEBUG, "Notifying:SQW:minion=%d:thread=%d\r\n", minion, thread);

    /* TODO: Today we are stuck here due to the reason that this primitive
    expects an ACK from the receiver end. Hence, it blocks the caller.
    There are two options in future: 1. To use IPIs.
    2. Improve FCC to address security concerns. */
    local_fcc_flag_notify_no_ack(&SQW_CB.sqw_fcc_flags[sqw_idx],
        minion, thread);

    return;
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
    static uint8_t cmd_buff[MM_CMD_MAX_SIZE] __attribute__((aligned(64))) = { 0 };
    struct cmd_header_t *cmd_hdr = (void*)cmd_buff;
    bool update_sq_tail;
    int8_t status = 0;
    int32_t pop_ret_val;
    uint32_t tail_prev;
    uint32_t vq_used_space;
    uint64_t start_cycles = 0;
    void* shared_mem_ptr;
    circ_buff_cb_t circ_buff_cached;
    vq_cb_t vq_cached;
    vq_cb_t *vq_shared = Host_Iface_Get_VQ_Base_Addr(SQ, (uint8_t)sqw_idx);

    /* TODO: This could also be achieved by using VQ CB as always cached,
    by locking the global variable in cache - VQ CB could be kept in SCP */
    /* Make a copy of the VQ CB in cached DRAM to cached L1 stack variable */
    ETSOC_Memory_Read_Local_Atomic((void*)vq_shared, (void*)&vq_cached, sizeof(vq_cached));

    /* Make a copy of the Circular Buffer CB in shared SRAM to cached L1 stack variable */
    ETSOC_Memory_Read_Uncacheable((void*)vq_cached.circbuff_cb, (void*)&circ_buff_cached,
        sizeof(circ_buff_cached));

    /* Save the shared memory pointer */
    shared_mem_ptr = (void*)vq_cached.circbuff_cb->buffer_ptr;

    /* Update the local VQ CB to point to the cached L1 stack variable */
    vq_cached.circbuff_cb = &circ_buff_cached;

    Log_Write(LOG_LEVEL_CRITICAL, "SQW:H%d:IDX=%d\r\n", hart_id, sqw_idx);

    while(1)
    {
        /* Wait for SQ Worker notification from Dispatcher */
        local_fcc_flag_wait(&SQW_CB.sqw_fcc_flags[sqw_idx]);

        /* Get current minion cycle */
        start_cycles = PMC_Get_Current_Cycles();

        Log_Write(LOG_LEVEL_DEBUG, "SQW:H%d:received FCC event!\r\n", hart_id);

        /* Reset the flag for updating the SQ tail offset in shared SRAM */
        update_sq_tail = false;

        /* Get the cached tail pointer */
        tail_prev = VQ_Get_Tail_Offset(&vq_cached);

        /* Refresh the cached VQ CB - Get updated head and tail values */
        VQ_Get_Head_And_Tail(vq_shared, &vq_cached);

        /* Verify that the tail value read from memory is equal to previous tail value */
        if(tail_prev == VQ_Get_Tail_Offset(&vq_cached))
        {
            /* Get the total number of bytes available in the VQ */
            vq_used_space = VQ_Get_Used_Space(&vq_cached, UNCACHED);

            /* Process commands until there is no more data in VQ */
            while(vq_used_space)
            {
                /* Pop from Submission Queue */
                pop_ret_val = VQ_Pop_Optimized(&vq_cached, vq_used_space, shared_mem_ptr, cmd_buff);

                if(pop_ret_val > 0)
                {
                    Log_Write(LOG_LEVEL_DEBUG, "SQW:Processing:SQW_IDX=%d:tag_id=%x\r\n",
                        sqw_idx, cmd_hdr->cmd_hdr.tag_id);

                    /* If barrier flag is set, wait until all cmds are
                    processed in the current SQ */
                    if(cmd_hdr->flags & (1 << 0U))
                    {
                        sqw_command_barrier((uint8_t)sqw_idx);
                    }

                    /* Increment the SQW command count.
                    NOTE: Its Host_Command_Handler's job to ensure this
                    count is decremented on the basis of the path it
                    takes to process a command. */
                    SQW_Increment_Command_Count((uint8_t)sqw_idx);

                    status = Host_Command_Handler(cmd_buff,
                        (uint8_t)sqw_idx, start_cycles);

                    if(status != STATUS_SUCCESS)
                    {
                        Log_Write(LOG_LEVEL_ERROR, "SQW:ERROR:Procesisng failed.(Error code: %d)\r\n",
                            status);
                    }
                }
                else if(pop_ret_val < 0)
                {
                    Log_Write(LOG_LEVEL_ERROR, "SQW:ERROR:VQ pop failed.(Error code: %d)\r\n",
                        pop_ret_val);
                }

                /* Set the SQ tail update flag so that we can updated shared
                memory circular buffer CB with new tail offset */
                update_sq_tail = true;

                /* Re-calculate the total number of bytes available in the VQ */
                vq_used_space = VQ_Get_Used_Space(&vq_cached, UNCACHED);
            }

            if(update_sq_tail)
            {
                /* Update the tail offset in VQ shared memory so that host is able to push new commands */
                Host_Iface_Optimized_SQ_Update_Tail(vq_shared, &vq_cached);
            }
        }
        else
        {
            /* TODO: Send an async event to host to inform about this fatal error */
            Log_Write(LOG_LEVEL_ERROR, "SQW:FATAL ERROR:Tail Mismatch:Cached: %d, Shared Memory: %d\r\n",
                tail_prev, VQ_Get_Tail_Offset(&vq_cached));
        }
    }

    return;
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
    int32_t original_val =
        atomic_add_signed_local_32(&SQW_CB.sqw_cmd_count[sqw_idx], -1);

    Log_Write(LOG_LEVEL_DEBUG, "SQW:Decrement:Command Count: %d\r\n", original_val - 1);
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
    int32_t original_val =
        atomic_add_signed_local_32(&SQW_CB.sqw_cmd_count[sqw_idx], 1);

    Log_Write(LOG_LEVEL_DEBUG, "SQW:Increment:Command Count: %d\r\n", original_val + 1);
}
