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
/*! \file sp_iface.c
    \brief A C module that implements the Service Processor Interface
    services

    Public interfaces:
        SP_Iface_Init
        SP_Iface_Processing
        SP_Iface_Get_Shire_Mask
        SP_Iface_Reset_Minion
        SP_Iface_Get_Boot_Freq
        SP_Iface_Report_Error
*/
/***********************************************************************/
/* mm_rt_svcs */
#include <etsoc/isa/sync.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/riscv_encoding.h>
#include <etsoc/isa/syscall.h>
#include <etsoc/drivers/pmu/pmu.h>
#include <system/layout.h>
#include <transports/sp_mm_iface/sp_mm_comms_spec.h>

/* mm specific headers */
#include "services/sp_iface.h"
#include "services/sw_timer.h"
#include "services/log.h"
#include "services/host_cmd_hdlr.h"
#include "workers/cw.h"

/* mm_rt_helpers */
#include "error_codes.h"
#include "syscall_internal.h"
#include "workers/kw.h"
#include "workers/dmaw.h"
#include "workers/sqw.h"

/*! \struct sp_iface_sq_cb_t
    \brief SP interface control block that manages
    submission queue
*/
typedef struct sp_iface_sq_cb_ {
    spinlock_t vq_lock;
    uint32_t timeout_flag;
} sp_iface_sq_cb_t;

/*! \var sp_iface_sq_cb_t SP_SQ_CB
    \brief Global SP to MM submission
    queue interface
    \warning Not thread safe!
*/
static sp_iface_sq_cb_t SP_SQ_CB __attribute__((aligned(64))) = { 0 };

/* Local prototypes */
static int32_t sp_command_handler(const void *cmd_buffer);
static int32_t wait_for_response_from_service_processor(void);

/************************************************************************
*
*   FUNCTION
*
*       sp_iface_mm_heartbeat_cb
*
*   DESCRIPTION
*
*       Callback to generate MM->SP heartbeat after every interval
*
***********************************************************************/
static void sp_iface_mm_heartbeat_cb(uint8_t param)
{
    struct mm2sp_heartbeat_event_t event;
    int32_t status;
    (void)param;

    /* Initialize event header */
    SP_MM_IFACE_INIT_MSG_HDR(&event.msg_hdr, MM2SP_EVENT_HEARTBEAT,
        sizeof(struct mm2sp_heartbeat_event_t), (int32_t)get_hart_id())

    /* Acquire the lock. Multiple threads can call this function. */
    acquire_local_spinlock(&SP_SQ_CB.vq_lock);

    /* Send event to Service Processor */
    status = SP_Iface_Push_Cmd_To_MM2SP_SQ(&event, sizeof(event));
    if ((status != STATUS_SUCCESS) && (status != CIRCBUFF_ERROR_FULL))
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: Pushing event to MM to SP SQ\r\n");
    }

    /* Release the lock */
    release_local_spinlock(&SP_SQ_CB.vq_lock);
}

/************************************************************************
*
*   FUNCTION
*
*       sp_iface_sp_response_timeout_cb
*
*   DESCRIPTION
*
*       Callback for SP response timer
*
***********************************************************************/
static void sp_iface_sp_response_timeout_cb(uint8_t thread_id)
{
    /* Set the timeout flag */
    atomic_store_local_32(&SP_SQ_CB.timeout_flag, 1);

    /* Trigger IPI to respective hart */
    syscall(SYSCALL_IPI_TRIGGER_INT, (1ULL << thread_id), MASTER_SHIRE, 0);
}

/************************************************************************
*
*   FUNCTION
*
*       wait_for_response_from_service_processor
*
*   DESCRIPTION
*
*       Helper to wait for notification events from service processor
*
***********************************************************************/
static int32_t wait_for_response_from_service_processor(void)
{
    uint64_t sip;
    int32_t sw_timer_idx;
    int32_t status = STATUS_SUCCESS;

    /* Create timeout for SP response */
    sw_timer_idx = SW_Timer_Create_Timeout(&sp_iface_sp_response_timeout_cb,
        (get_hart_id() & (HARTS_PER_SHIRE - 1)), TIMEOUT_SP_IFACE_RESPONSE(5));

    /* If the timer was successfully registered */
    if (sw_timer_idx < 0)
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: Unable to register SP response timeout!\r\n");
        return SP_IFACE_TIMER_REGISTER_FAILED;
    }

    Log_Write(LOG_LEVEL_DEBUG, "Waiting on SP\r\n");

    while (1)
    {
        /* Wait for an interrupt */
        asm volatile("wfi");

        /* Read pending interrupts */
        SUPERVISOR_PENDING_INTERRUPTS(sip);

        Log_Write(LOG_LEVEL_DEBUG, "Waiting on SP: received an interrupt 0x%" PRIx64 "\r\n", sip);

        if (sip & (1 << SUPERVISOR_SOFTWARE_INTERRUPT))
        {
            /* Clear IPI pending interrupt */
            asm volatile("csrc sip, %0" : : "r"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

            /* Free the registered SW Timeout slot */
            SW_Timer_Cancel_Timeout((uint8_t)sw_timer_idx);

            /* Check for timeout flag */
            if (atomic_compare_and_exchange_local_32(&SP_SQ_CB.timeout_flag, 1, 0) == 1)
            {
                status = SP_IFACE_SP_RSP_TIMEDOUT;
                Log_Write(LOG_LEVEL_ERROR, "Error:Timed-out waiting for response from SP\r\n");
            }
            break;
        }
        else
        {
            /* We are only interested in IPIs */
            continue;
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       tf_command_handler
*
*   DESCRIPTION
*
*       Simply tunnel command to MM command handler
*
***********************************************************************/
static int32_t tf_command_handler(void *cmd_buffer)
{
    int32_t status = STATUS_SUCCESS;
    const struct cmd_header_t *hdr = cmd_buffer;

    Log_Write(LOG_LEVEL_DEBUG, "SP2MM:CMD:TF_Command_Handler. \r\n");

    /* Check for abort command */
    if (hdr->cmd_hdr.msg_id == DEV_OPS_API_MID_DEVICE_OPS_ABORT_CMD)
    {
        status = Host_HP_Command_Handler(cmd_buffer, 0);
    }
    else
    {
        /* Process command and pass current minion cycle
        For TF, assume the SQW index as zero. */
        status = Host_Command_Handler(cmd_buffer, 0, PMC_Get_Current_Cycles());
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       sp_command_handler
*
*   DESCRIPTION
*
*       A local scope helper function to process supported SP to MM
*       commands
*
***********************************************************************/
static int32_t sp_command_handler(const void *cmd_buffer)
{
    int32_t status = STATUS_SUCCESS;
    const struct dev_cmd_hdr_t *hdr = cmd_buffer;

    Log_Write(LOG_LEVEL_DEBUG, "SP2MM:CMD:SP_Command_Handler:hdr:%s%d%s%d%s",
        ":msg_id:", hdr->msg_id, ":msg_size:", hdr->msg_size, "\r\n");

    switch (hdr->msg_id)
    {
        case SP2MM_CMD_ECHO:
        {
            const struct sp2mm_echo_cmd_t *echo_cmd = (const void *)hdr;
            struct sp2mm_echo_rsp_t rsp;

            Log_Write(LOG_LEVEL_DEBUG, "SP_Command_Handler:Echo:%s%d%s%d%s0x%x%s",
                ":msg_id:", hdr->msg_id, ":msg_size:", hdr->msg_size, ":payload", echo_cmd->payload,
                "\r\n");

            /* Initialize response header */
            SP_MM_IFACE_INIT_MSG_HDR(&rsp.msg_hdr, SP2MM_RSP_ECHO, sizeof(struct sp2mm_echo_rsp_t),
                echo_cmd->msg_hdr.issuing_hart_id)

            rsp.payload = echo_cmd->payload;

            status = SP_Iface_Push_Rsp_To_SP2MM_CQ((void *)&rsp, sizeof(rsp));
            if (status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG,
                    "MM2SP:RSP:SP_Iface_Push_Cmd_To_SP2MM_CQ: CQ push success!\r\n");
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR, "SP_Iface_Push_Cmd_To_SP2MM_CQ: CQ push error!\r\n");
            }

            break;
        }
        case SP2MM_CMD_UPDATE_FREQ:
        {
            const struct sp2mm_update_freq_cmd_t *update_active_freq_cmd = (const void *)hdr;

            Log_Write(LOG_LEVEL_DEBUG,
                "SP2MM:CMD:SP_Command_Handler:UpdateActiveFreq:%s%d%s%d%s0x%x%s",
                ":msg_id:", hdr->msg_id, ":msg_size:", hdr->msg_size, ":payload",
                update_active_freq_cmd->freq, "\r\n");

            /* Update Compute Workers frequency */
            syscall(SYSCALL_UPDATE_MINION_PLL_FREQUENCY, update_active_freq_cmd->freq, 0, 0);

            break;
        }
        case SP2MM_CMD_TEARDOWN_MM:
        {
            /* struct sp2mm_teardown_mm_cmd_t *teardown_mm_cmd = (const void*) hdr */

            Log_Write(LOG_LEVEL_DEBUG, "SP2MM:CMD:SP_Command_Handler:TearDownMM:%s%d%s%d%s",
                ":msg_id:", hdr->msg_id, ":msg_size:", hdr->msg_size, "\r\n");

            /* Implement functionality here .. */
            break;
        }
        case SP2MM_CMD_QUIESCE_TRAFFIC:
        {
            /* struct sp2mm_quiesce_traffic_cmd_t *quiese_traffic_cmd = (const void*) hdr */

            Log_Write(LOG_LEVEL_DEBUG, "SP2MM:CMD:SP_Command_Handler:QuieseTraffic:%s%d%s%d%s",
                ":msg_id:", hdr->msg_id, ":msg_size:", hdr->msg_size, "\r\n");

            /* Implement functionality here .. */
            break;
        }
        case SP2MM_CMD_MM_ABORT_ALL:
        {
            struct sp2mm_mm_abort_all_rsp_t rsp;
            Log_Write(LOG_LEVEL_DEBUG, "SP2MM:CMD:SP_Command_Handler:MM Abort:%s%d%s%d%s",
                ":msg_id:", hdr->msg_id, ":msg_size:", hdr->msg_size, "\r\n");

            /* Commands abort will be done in 3 phases:
            1. Abort any pending commands in a particular SQ
            2. Abort any dispatched DMA read/write commands for the particular SQ
            3. Abort any dispatched Kernel command for the particular SQ
            abort all commands on all SQs */
            for (uint8_t sq_idx = 0; sq_idx < MM_SQ_COUNT; sq_idx++)
            {
                /* Blocking call that aborts all pending commands in the paired normal SQ */
                SQW_Abort_All_Pending_Commands(sq_idx);

                /* Blocking call that aborts all DMA read channels */
                DMAW_Abort_All_Dispatched_Read_Channels(sq_idx);

                /* Blocking call that aborts all DMA write channels */
                DMAW_Abort_All_Dispatched_Write_Channels(sq_idx);

                /* Blocking call that aborts all dispatched kernels */
                KW_Abort_All_Dispatched_Kernels(sq_idx);
            }

            /* Initialize response header */
            SP_MM_IFACE_INIT_MSG_HDR(
                &rsp.msg_hdr, SP2MM_RSP_MM_ABORT_ALL, sizeof(struct sp2mm_mm_abort_all_rsp_t), 0)

            rsp.status = DEV_OPS_API_ABORT_RESPONSE_SUCCESS;

            status = SP_Iface_Push_Rsp_To_SP2MM_CQ((void *)&rsp, sizeof(rsp));
            if (status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG,
                    "MM2SP:RSP:SP2MM_CMD_MM_ABORT_ALL:SP_Iface_Push_Cmd_To_SP2MM_CQ: CQ push success!\r\n");
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "SP2MM_CMD_MM_ABORT_ALL:SP_Iface_Push_Cmd_To_SP2MM_CQ: CQ push error!\r\n");
            }
        }
        break;
        case SP2MM_CMD_CM_RESET:
        {
            struct sp2mm_cm_reset_rsp_t rsp;

            /* Perform CM warm reset and wait*/
            status = CW_CM_Configure_And_Wait_For_Boot();

            /* Initialize response header */
            SP_MM_IFACE_INIT_MSG_HDR(
                &rsp.msg_hdr, SP2MM_RSP_CM_RESET, sizeof(struct sp2mm_cm_reset_rsp_t), 0)

            rsp.status = status;

            status = SP_Iface_Push_Rsp_To_SP2MM_CQ((void *)&rsp, sizeof(rsp));
            if (status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG,
                    "MM2SP:RSP:SP2MM_CMD_MM_RESET_ALL:SP_Iface_Push_Cmd_To_SP2MM_CQ: CQ push success!\r\n");
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "SP2MM_CMD_MM_RESET_ALL:SP_Iface_Push_Cmd_To_SP2MM_CQ: CQ push error!\r\n");
            }
        }
        break;
        default:
        {
            Log_Write(LOG_LEVEL_ERROR, "SP_Command_Handler:UnsupportedCommandID.\r\n");

            break;
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_Init
*
*   DESCRIPTION
*
*       Initialize the SP interface for communication.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int32_t      status success or failure of Interface initialization
*
***********************************************************************/
int32_t SP_Iface_Init(void)
{
    int32_t status;

    status = SP_MM_Iface_Init();

    if (status == STATUS_SUCCESS)
    {
        /* Initialize the lock for SP SQ */
        init_local_spinlock(&SP_SQ_CB.vq_lock, 0);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_Processing
*
*   DESCRIPTION
*
*       Process MM2SP CQ response messages, and SP2MM SQ command messages
*       from Service Processor (SP)
*
*   INPUTS
*
*       vq_cached      Pointer to cached virtual queue control block
*       vq_shared      Pointer to shared virtual queue control block
*       shared_mem_ptr Pointer to shared circular buffer pointer
*       vq_used_space  Number of bytes available to pop
*
*   OUTPUTS
*
*       int32_t      status success or failure of Interface initialization
*
***********************************************************************/
int32_t SP_Iface_Processing(
    vq_cb_t *vq_cached, vq_cb_t *vq_shared, void *shared_mem_ptr, uint64_t vq_used_space)
{
    static uint8_t cmd_buff[64] __attribute__((aligned(64))) = { 0 };
    int32_t cmd_length = 0;
    int32_t status = STATUS_SUCCESS;

    Log_Write(LOG_LEVEL_DEBUG, "SP_Iface_Processing..\r\n");

    /* Create a shadow copy of data from SQ to L2 SCP */
    cmd_length = VQ_Pop_Optimized(vq_cached, vq_used_space, shared_mem_ptr, cmd_buff);

    /* Update tail value in VQ memory */
    VQ_Set_Tail_Offset(vq_shared, VQ_Get_Tail_Offset(vq_cached));

    if (cmd_length > 0)
    {
        Log_Write(LOG_LEVEL_DEBUG, "SP_Iface_Processing:cmd_length: 0x%" PRIx32 "\r\n", cmd_length);
#if TEST_FRAMEWORK
        status = tf_command_handler(cmd_buff);
        (void)&sp_command_handler;
#else
        status = sp_command_handler(cmd_buff);
        (void)&tf_command_handler;
#endif
    }
    else if (cmd_length < 0)
    {
        Log_Write(LOG_LEVEL_ERROR, "SP_Iface_Processing:Failed: %" PRIi32 "\r\n", cmd_length);
        status = SP_IFACE_SP2MM_CMD_POP_FAILED;
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_Get_Shire_Mask
*
*   DESCRIPTION
*
*       A blocking call to obtain shire mask from SP
*
*   INPUTS
*
*       shire_mask  reference to shire mask to be used as return arg
*       lvdpll_trap  reference to lvdpll starp to be used as return arg
*
*   OUTPUTS
*
*       int32_t      status success or failure of Interface initialization
*
***********************************************************************/
int32_t SP_Iface_Get_Shire_Mask_And_Strap(uint64_t *shire_mask, uint8_t *lvdpll_strap)
{
    static uint8_t rsp_buff[64] __attribute__((aligned(64))) = { 0 };
    struct dev_cmd_hdr_t *hdr;
    uint64_t rsp_length = 0;
    struct mm2sp_get_active_shire_mask_cmd_t cmd;
    int32_t status = STATUS_SUCCESS;

    Log_Write(LOG_LEVEL_DEBUG, "MM2SP:SP_Iface_Get_Shire_Mask.\r\n");

    /* Initialize command header */
    SP_MM_IFACE_INIT_MSG_HDR(&cmd.msg_hdr, MM2SP_CMD_GET_ACTIVE_SHIRE_MASK,
        sizeof(struct mm2sp_get_active_shire_mask_cmd_t), (int32_t)get_hart_id())

    /* Acquire the lock. Multiple threads can call this function. */
    acquire_local_spinlock(&SP_SQ_CB.vq_lock);

    /* Send command to Service Processor */
    status = SP_Iface_Push_Cmd_To_MM2SP_SQ(&cmd, sizeof(cmd));

    if (status == STATUS_SUCCESS)
    {
        /* Wait for response from Service Processor */
        status = wait_for_response_from_service_processor();
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: Pushing command to MM to SP SQ\r\n");
    }

    if (status == STATUS_SUCCESS)
    {
        /* Pop response from MM to SP completion queue */
        rsp_length = (uint64_t)SP_Iface_Pop_Rsp_From_MM2SP_CQ(&rsp_buff[0]);

        /* Process response and fetch shire mask */
        if (rsp_length != 0)
        {
            Log_Write(
                LOG_LEVEL_DEBUG, "SP2MM:received response of size %ld bytes.\r\n", rsp_length);

            hdr = (void *)rsp_buff;
            if (hdr->msg_id == MM2SP_RSP_GET_ACTIVE_SHIRE_MASK)
            {
                const struct mm2sp_get_active_shire_mask_rsp_t *rsp = (void *)rsp_buff;
                *shire_mask = rsp->active_shire_mask;
                *lvdpll_strap = rsp->lvdpll_strap;
            }
            else
            {
                status = SP_IFACE_INVALID_SHIRE_MASK;
                Log_Write(LOG_LEVEL_ERROR,
                    "ERROR: Received unexpected response, for shire mask from SP\r\n");
            }
        }
        else
        {
            status = SP_IFACE_SP2MM_RSP_POP_FAILED;
            Log_Write(LOG_LEVEL_ERROR, "ERROR: Received a notification from SP with no data\r\n");
        }
    }

    /* Release the lock */
    release_local_spinlock(&SP_SQ_CB.vq_lock);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_Reset_Minion
*
*   DESCRIPTION
*
*       A blocking call to reset the Minions.
*
*   INPUTS
*
*       shire_mask  the mask of the shires to reset
*
*   OUTPUTS
*
*       int32_t      status success or failure of Interface initialization
*
***********************************************************************/
int32_t SP_Iface_Reset_Minion(uint64_t shire_mask)
{
    static uint8_t rsp_buff[64] __attribute__((aligned(64))) = { 0 };
    const struct dev_cmd_hdr_t *hdr;
    uint64_t rsp_length = 0;
    struct mm2sp_reset_minion_cmd_t cmd;
    int32_t status = STATUS_SUCCESS;

    Log_Write(LOG_LEVEL_DEBUG, "MM2SP:SP_Iface_Reset_Minion.\r\n");

    /* Initialize command header */
    SP_MM_IFACE_INIT_MSG_HDR(&cmd.msg_hdr, MM2SP_CMD_RESET_MINION,
        sizeof(struct mm2sp_reset_minion_cmd_t), (int32_t)get_hart_id())

    cmd.shire_mask = shire_mask;

    /* Acquire the lock. Multiple threads can call this function. */
    acquire_local_spinlock(&SP_SQ_CB.vq_lock);

    uint32_t start_time = SW_Timer_Get_Elapsed_Time();

    /* Wait for a timeout - required in cases when vq is drained due to multiple error
       events, let the SP process all the outstanding events */
    while (SW_Timer_Get_Elapsed_Time() - start_time < TIMEOUT_SP_IFACE_RESPONSE(50))
    {
        /* Send command to Service Processor */
        status = SP_Iface_Push_Cmd_To_MM2SP_SQ(&cmd, sizeof(cmd));

        if (status == STATUS_SUCCESS)
        {
            /* Wait for response from Service Processor */
            status = wait_for_response_from_service_processor();
        }
        else
        {
            Log_Write(LOG_LEVEL_ERROR, "ERROR: Pushing command to MM to SP SQ\r\n");
        }

        if (status != CIRCBUFF_ERROR_FULL)
        {
            break;
        }
    }

    if (status == STATUS_SUCCESS)
    {
        /* Pop response from MM to SP completion queue */
        rsp_length = (uint64_t)SP_Iface_Pop_Rsp_From_MM2SP_CQ(&rsp_buff[0]);

        /* Process response and fetch shire mask */
        if (rsp_length != 0)
        {
            Log_Write(
                LOG_LEVEL_DEBUG, "SP2MM:received response of size %ld bytes.\r\n", rsp_length);

            hdr = (void *)rsp_buff;
            if (hdr->msg_id == MM2SP_RSP_RESET_MINION)
            {
                const struct mm2sp_reset_minion_rsp_t *rsp = (void *)rsp_buff;
                status = rsp->results;
            }
            else
            {
                status = SP_IFACE_INVALID_RSP_ID;
                Log_Write(LOG_LEVEL_ERROR, "ERROR: Received unexpected response from SP\r\n");
            }
        }
        else
        {
            status = SP_IFACE_SP2MM_RSP_POP_FAILED;
            Log_Write(LOG_LEVEL_ERROR, "ERROR: Received a notification from SP with no data\r\n");
        }
    }

    /* Release the lock */
    release_local_spinlock(&SP_SQ_CB.vq_lock);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_Get_Boot_Freq
*
*   DESCRIPTION
*
*       A blocking call to obtain compute minions boot frequency from SP
*
*   INPUTS
*
*       shire_mask  Reference to boot frequency variable to be used as
*                   return arg
*
*   OUTPUTS
*
*       int32_t      status success or failure of Interface initialization
*
***********************************************************************/
int32_t SP_Iface_Get_Boot_Freq(uint32_t *boot_freq)
{
    static uint8_t rsp_buff[64] __attribute__((aligned(64))) = { 0 };
    struct dev_cmd_hdr_t *hdr;
    uint64_t rsp_length = 0;
    struct mm2sp_get_cm_boot_freq_cmd_t cmd;
    int32_t status = STATUS_SUCCESS;

    Log_Write(LOG_LEVEL_DEBUG, "MM2SP:SP_Iface_Get_Boot_Freq.\r\n");

    /* Initialize command header */
    SP_MM_IFACE_INIT_MSG_HDR(&cmd.msg_hdr, MM2SP_CMD_GET_CM_BOOT_FREQ,
        sizeof(struct mm2sp_get_cm_boot_freq_cmd_t), (int32_t)get_hart_id())

    /* Acquire the lock. Multiple threads can call this function. */
    acquire_local_spinlock(&SP_SQ_CB.vq_lock);

    /* Send command to Service Processor */
    status = SP_Iface_Push_Cmd_To_MM2SP_SQ(&cmd, sizeof(cmd));

    if (status == STATUS_SUCCESS)
    {
        /* Wait for response from Service Processor */
        status = wait_for_response_from_service_processor();
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: Pushing command to MM to SP SQ\r\n");
    }

    if (status == STATUS_SUCCESS)
    {
        /* Pop response from MM to SP completion queue */
        rsp_length = (uint64_t)SP_Iface_Pop_Rsp_From_MM2SP_CQ(&rsp_buff[0]);

        /* Process response and fetch boot frequency */
        if (rsp_length != 0)
        {
            Log_Write(
                LOG_LEVEL_DEBUG, "SP2MM:received response of size %ld bytes.\r\n", rsp_length);

            hdr = (void *)rsp_buff;
            if (hdr->msg_id == MM2SP_RSP_GET_CM_BOOT_FREQ)
            {
                const struct mm2sp_get_cm_boot_freq_rsp_t *rsp = (void *)rsp_buff;
                *boot_freq = rsp->cm_boot_freq;
            }
            else
            {
                status = SP_IFACE_INVALID_BOOT_FREQ;
                Log_Write(LOG_LEVEL_ERROR,
                    "ERROR: Received unexpected response, for boot frequency from SP \r\n");
            }
        }
        else
        {
            status = SP_IFACE_SP2MM_RSP_POP_FAILED;
            Log_Write(LOG_LEVEL_ERROR, "ERROR: Received a notification from SP and no data\r\n");
        }
    }

    /* Release the lock */
    release_local_spinlock(&SP_SQ_CB.vq_lock);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_Get_Fw_Version
*
*   DESCRIPTION
*
*       A blocking call to obtain FW version from SP
*
*   INPUTS
*
*       fw_type     Type of FW
*       major       Pointer to variable where major version is stored
*       minor       Pointer to variable where minor version is stored
*       revision    Pointer to variable where revision version is stored
*
*   OUTPUTS
*
*       int32_t      status success or failure of Interface initialization
*
***********************************************************************/
int32_t SP_Iface_Get_Fw_Version(
    mm2sp_fw_type_e fw_type, uint8_t *major, uint8_t *minor, uint8_t *revision)
{
    uint8_t rsp_buff[64] __attribute__((aligned(64))) = { 0 };
    const struct dev_cmd_hdr_t *hdr;
    uint64_t rsp_length = 0;
    struct mm2sp_get_fw_version_t cmd;
    int32_t status = STATUS_SUCCESS;

    /* Set the firmware type */
    cmd.fw_type = fw_type;

    Log_Write(LOG_LEVEL_DEBUG, "MM2SP:SP_Iface_Get_Fw_Version.\r\n");

    /* Initialize command header */
    SP_MM_IFACE_INIT_MSG_HDR(&cmd.msg_hdr, MM2SP_CMD_GET_FW_VERSION,
        sizeof(struct mm2sp_get_fw_version_t), (int32_t)get_hart_id())

    /* Acquire the lock. Multiple threads can call this function. */
    acquire_local_spinlock(&SP_SQ_CB.vq_lock);

    /* Send command to Service Processor */
    status = SP_Iface_Push_Cmd_To_MM2SP_SQ(&cmd, sizeof(cmd));

    if (status == STATUS_SUCCESS)
    {
        /* Wait for response from Service Processor */
        status = wait_for_response_from_service_processor();
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: Pushing command to MM to SP SQ\r\n");
    }

    if (status == STATUS_SUCCESS)
    {
        /* Pop response from MM to SP completion queue */
        rsp_length = (uint64_t)SP_Iface_Pop_Rsp_From_MM2SP_CQ(&rsp_buff[0]);

        /* Process response and fetch shire mask */
        if (rsp_length != 0)
        {
            Log_Write(
                LOG_LEVEL_DEBUG, "SP2MM:received response of size %ld bytes.\r\n", rsp_length);

            hdr = (void *)rsp_buff;
            if (hdr->msg_id == MM2SP_RSP_GET_FW_VERSION)
            {
                const struct mm2sp_get_fw_version_rsp_t *rsp = (void *)rsp_buff;
                *major = rsp->major;
                *minor = rsp->minor;
                *revision = rsp->revision;
            }
            else
            {
                status = SP_IFACE_INVALID_FW_VERSION;
                Log_Write(LOG_LEVEL_ERROR,
                    "ERROR: Received unexpected response, for fw version from SP\r\n");
            }
        }
        else
        {
            status = SP_IFACE_SP2MM_RSP_POP_FAILED;
            Log_Write(LOG_LEVEL_ERROR, "ERROR: Received a notification from SP with no data\r\n");
        }
    }

    /* Release the lock */
    release_local_spinlock(&SP_SQ_CB.vq_lock);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_Report_Error
*
*   DESCRIPTION
*
*       A non-blocking call to report error to Service Processor
*
*   INPUTS
*
*       error_type   Type of error
*       error_code   Error code to report
*
*   OUTPUTS
*
*       int32_t       status success or failure
*
***********************************************************************/
int32_t SP_Iface_Report_Error(mm2sp_error_type_e error_type, int16_t error_code)
{
    struct mm2sp_report_error_event_t event;
    int32_t status = STATUS_SUCCESS;

    /* Initialize event header */
    SP_MM_IFACE_INIT_MSG_HDR(&event.msg_hdr, MM2SP_EVENT_REPORT_ERROR,
        sizeof(struct mm2sp_report_error_event_t), (int32_t)get_hart_id())
    event.error_type = error_type;
    event.error_code = error_code;

    /* Acquire the lock. Multiple threads can call this function. */
    acquire_local_spinlock(&SP_SQ_CB.vq_lock);

    /* Send event to Service Processor */
    status = SP_Iface_Push_Cmd_To_MM2SP_SQ(&event, sizeof(event));
    if (status != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "ERROR: Pushing event to MM to SP SQ:%d\r\n", status);
    }

    /* Release the lock */
    release_local_spinlock(&SP_SQ_CB.vq_lock);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       SP_Iface_Setup_MM_HeartBeat
*
*   DESCRIPTION
*
*       This function initializes the MM->SP heartbeat with a periodic
*       timer.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       int32_t       status success or failure
*
***********************************************************************/
int32_t SP_Iface_Setup_MM_HeartBeat(void)
{
    int32_t sw_timer_idx;
    int32_t status = STATUS_SUCCESS;

    /* Create timer for MM heartbeat */
    /* TODO: Fine tune the heartbeat interval */
    sw_timer_idx =
        SW_Timer_Create_Timeout(&sp_iface_mm_heartbeat_cb, 0, SP_IFACE_MM_HEARTBEAT_INTERVAL(100));

    /* If the timer was not successfully registered */
    if (sw_timer_idx < 0)
    {
        status = SP_IFACE_TIMER_REGISTER_FAILED;
        Log_Write(LOG_LEVEL_ERROR, "ERROR: Unable to register MM->SP Heartbeat Timer!\r\n");
    }

    return status;
}
