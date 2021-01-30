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
#include "services/log1.h"
#include "services/host_iface.h"
#include "services/host_cmd_hdlr.h"
#include <esperanto/device-apis/device_apis_message_types.h>
#include "pmu.h"

/*! \typedef sqw_cb_t
    \brief Submission Queue Worker Control Block structure
*/
typedef struct sqw_cb_ {
    int32_t             sqw_cmd_count[MM_SQ_COUNT];
    global_fcc_flag_t   sqw_fcc_flags[MM_SQ_COUNT];
    vq_cb_t             *sq[MM_SQ_COUNT];
} sqw_cb_t;

/*! \var sqw_cb_t SQW_CB
    \brief Global Submission Queue Worker Control Block
    \warning Not thread safe!
*/
static sqw_cb_t SQW_CB __attribute__((aligned(64))) = {0};

extern spinlock_t Launch_Lock;

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
    Log_Write(LOG_LEVEL_DEBUG, "%s", "SQW:Command Barrier\r\n");

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
        global_fcc_flag_init(&SQW_CB.sqw_fcc_flags[i]);

        atomic_store_local_32((uint32_t*)&SQW_CB.sqw_cmd_count[i], 0U);

        atomic_store_local_64((uint64_t*)&SQW_CB.sq[i],
            (uint64_t)(void*) Host_Iface_Get_VQ_Base_Addr(SQ, i));
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

    Log_Write(LOG_LEVEL_DEBUG,
        "%s%d%s%d%s","Notifying:SQW:minion=", minion, ":thread=",
        thread, "\r\n");

    global_fcc_flag_notify(&SQW_CB.sqw_fcc_flags[sqw_idx],
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
    static uint8_t
        cmd_buff[MM_CMD_MAX_SIZE] __attribute__((aligned(8))) = { 0 };
    struct cmd_header_t *cmd_hdr = (void*)cmd_buff;
    int8_t status = 0;
    int32_t pop_ret_val;
    uint64_t start_cycles=0;

    /* Release the launch lock to let other workers acquire it */
    release_local_spinlock(&Launch_Lock);

    Log_Write(LOG_LEVEL_CRITICAL, "%s%d%s%d%s",
        "SQW:HART=", hart_id, ":IDX=", sqw_idx, "\r\n");

    /* Empty all FCCs */
    init_fcc(FCC_0);
    init_fcc(FCC_1);

    while(1)
    {
        /* Wait for SQ Worker notification from Dispatcher*/
        global_fcc_flag_wait(&SQW_CB.sqw_fcc_flags[sqw_idx]);

	    /* Get current minion cycle */
        start_cycles = PMC_Get_Current_Cycles();

        Log_Write(LOG_LEVEL_DEBUG, "%s%d%s",
            "SQW:HART:", hart_id, ":received FCC event!\r\n");

        /* Process commands until there is no more data */
        do
        {
            /* Pop from Submission Queue */
            pop_ret_val = VQ_Pop(SQW_CB.sq[sqw_idx], cmd_buff);

            if(pop_ret_val > 0)
            {
                Log_Write(LOG_LEVEL_DEBUG, "%s%d%s",
                    "SQW:Processing:SQW_IDX=",
                    sqw_idx, "\r\n");

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

                if (status != STATUS_SUCCESS)
                {
                    Log_Write(LOG_LEVEL_ERROR, "%s %d %s",
                        "SQW:ERROR:Procesisng failed.(Error code:)",
                        status, "\r\n");
                }
            }
            else if (pop_ret_val < 0)
            {
                Log_Write(LOG_LEVEL_ERROR, "%s%d%s",
                    "SQW:ERROR:VQ pop failed.(Error code:)",
                    pop_ret_val, "\r\n");
            }
        } while (pop_ret_val > 0);
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
    atomic_add_signed_local_32(&SQW_CB.sqw_cmd_count[sqw_idx], -1);

    /* sqw_cmd_count value being shown here is not garanteed to be
       thread safe. */
    Log_Write(LOG_LEVEL_DEBUG, "%s%d%s",
        "SQW:Decrement:Command Count:",
        atomic_load_local_32((uint32_t*)&SQW_CB.sqw_cmd_count[sqw_idx]),
        "\r\n");
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
    atomic_add_signed_local_32(&SQW_CB.sqw_cmd_count[sqw_idx], 1);

    /* sqw_cmd_count value being shown here is not garanteed to be
       thread safe. */
    Log_Write(LOG_LEVEL_DEBUG, "%s%d%s",
        "SQW:Increment:Command Count:",
        atomic_load_local_32((uint32_t*)&SQW_CB.sqw_cmd_count[sqw_idx]),
        "\r\n");
}
