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
/*! \file kw.c
    \brief A C module that implements the Kernel Worker's
    public and private interfaces. The kernel worker implements;
    1. KW_Launch - An infinite loop that unblocks on FCC notification
    from SQWs, aggregates kernel launch responses from compute workers,
    constructs and transmits launch response to host
    2. It implements, and exposes the below listed public interfaces to
    other master shire runtime components in the system
    to facilitate kernel management

    Public interfaces:
        KW_Init
        KW_Notify
        KW_Launch
        KW_Dispatch_Kernel_Launch_Cmd
        KW_Dispatch_Kernel_Abort_Cmd
*/
/***********************************************************************/
#include    "atomic.h"
#include    "common_defs.h"
#include    "config/mm_config.h"
#include    "services/host_iface.h"
#include    "services/cm_iface.h"
#include    "services/log.h"
#include    "services/sw_timer.h"
#include    "workers/cw.h"
#include    "workers/kw.h"
#include    "workers/sqw.h"
#include    "cacheops.h"
#include    "circbuff.h"
#include    "riscv_encoding.h"
#include    "sync.h"
#include    "utils.h"
#include    "vq.h"
#include    "syscall_internal.h"

/*! \typedef kernel_instance_t
    \brief Kernel Instance Control Block structure.
    Kernel instance maintains information related to
    a given kernel launch for the life time of kernel
    launch command.
*/
typedef struct kernel_instance_ {
    tag_id_t launch_tag_id;
    uint16_t sqw_idx;
    uint16_t kernel_state;
    uint8_t  sw_timer_idx;
    uint8_t  reserved;
    exec_cycles_t kw_cycles;
    uint64_t kernel_shire_mask;
} kernel_instance_t;

/*! \typedef kw_cb_t
    \brief Kernel Worker Control Block structure.
    Used to maintain kernel instance and related resources
    for the life time of MM runtime.
*/
typedef struct kw_cb_ {
    fcc_sync_cb_t       host2kw[MM_MAX_PARALLEL_KERNELS];
    spinlock_t          resource_lock;
    kernel_instance_t   kernels[MM_MAX_PARALLEL_KERNELS];
} kw_cb_t;

/*! \var kw_cb_t KW_CB
    \brief Global Kernel Worker Control Block
    \warning Not thread safe!
*/
static kw_cb_t KW_CB __attribute__((aligned(64))) = {0};

/* Local function prototypes */

static inline bool kw_check_address_bounds(uint64_t dev_address)
{
    return ((dev_address >= HOST_MANAGED_DRAM_START) &&
            (dev_address < HOST_MANAGED_DRAM_END));
}

/************************************************************************
*
*   FUNCTION
*
*       kw_find_used_kernel_slot
*
*   DESCRIPTION
*
*       Local fn helper to find used kernel slot
*
*   INPUTS
*
*       launch_tag_id  Tag ID of the launched kernel to find
*       slot           Pointer to return the found kernel slot
*
*   OUTPUTS
*
*       int8_t         status success or error
*
***********************************************************************/
static int8_t kw_find_used_kernel_slot(uint16_t launch_tag_id, uint8_t *slot)
{
    int8_t status = KW_ERROR_KERNEL_SLOT_NOT_FOUND;

    /* Acquire the lock */
    acquire_local_spinlock(&KW_CB.resource_lock);

    for(uint8_t i = 0; i < MM_MAX_PARALLEL_KERNELS; i++)
    {
        /* Find the kernel with the given tag ID */
        if(atomic_load_local_16(&KW_CB.kernels[i].launch_tag_id) ==
            launch_tag_id)
        {
            /* Check if the slot is in use */
            if(atomic_load_local_16
                (&KW_CB.kernels[i].kernel_state) == KERNEL_STATE_IN_USE)
            {
                *slot = i;
                status = STATUS_SUCCESS;
            }
            else
            {
                status = KW_ERROR_KERNEL_SLOT_NOT_USED;
            }
            break;
        }
    }

    /* Release the lock */
    release_local_spinlock(&KW_CB.resource_lock);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       kw_reserve_kernel_slot
*
*   DESCRIPTION
*
*       Local fn helper to reserve kernel slot
*
*   INPUTS
*
*       slot_index          Index of available KW
*
*   OUTPUTS
*
*       kernel_instance_t*  Reference to kernel instance
*
***********************************************************************/
static kernel_instance_t* kw_reserve_kernel_slot(uint8_t *slot_index)
{
    kernel_instance_t* kernel = 0;

    /* Acquire the lock */
    acquire_local_spinlock(&KW_CB.resource_lock);

    for(uint8_t i = 0; i < MM_MAX_PARALLEL_KERNELS; i++)
    {
        if(atomic_load_local_16
            (&KW_CB.kernels[i].kernel_state) == KERNEL_STATE_UN_USED)
        {
            kernel = &KW_CB.kernels[i];
            atomic_store_local_16(&kernel->kernel_state,
                KERNEL_STATE_IN_USE);
            *slot_index = i;
            break;
        }
    }

    /* Release the lock */
    release_local_spinlock(&KW_CB.resource_lock);

    return kernel;
}

/************************************************************************
*
*   FUNCTION
*
*       kw_unreserve_kernel_slot
*
*   DESCRIPTION
*
*       Local fn helper to mark kernel slot as free.
*
*   INPUTS
*
*       kernel  Reference to kernel reference
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void kw_unreserve_kernel_slot(kernel_instance_t* kernel)
{
    /* Acquire the lock */
    acquire_local_spinlock(&KW_CB.resource_lock);

    atomic_store_local_16(&kernel->kernel_state, KERNEL_STATE_UN_USED);

    /* Release the lock */
    release_local_spinlock(&KW_CB.resource_lock);
}

/************************************************************************
*
*   FUNCTION
*
*       kw_reserve_kernel_shires
*
*   DESCRIPTION
*
*       Local fn helper to mark compute minions in-use. The shire mask
*       provided by caller determines the minions to be marked in-use.
*
*   INPUTS
*
*       req_shire_mask  Shire mask of compute minions to be marked in-use
*
*   OUTPUTS
*
*       int8_t          status success or error
*
***********************************************************************/
static int8_t kw_reserve_kernel_shires(uint64_t req_shire_mask)
{
    int8_t status;

    /* Acquire the lock */
    acquire_local_spinlock(&KW_CB.resource_lock);

    /* Check the required shires are available and ready */
    status = CW_Check_Shires_Available_And_Ready(req_shire_mask);

    if(status == STATUS_SUCCESS)
    {
        /* Update the each shire status to reserved */
        for (uint64_t shire = 0; shire < NUM_SHIRES; shire++)
        {
            if (req_shire_mask & (1ULL << shire))
            {
                CW_Update_Shire_State(shire, CW_SHIRE_STATE_RESERVED);
            }
        }
    }
    else if (status == CW_SHIRES_NOT_READY)
    {
        status = KW_ERROR_KERNEL_SHIRES_NOT_READY;
    }

    /* Release the lock */
    release_local_spinlock(&KW_CB.resource_lock);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       kw_unreserve_kernel_shires
*
*   DESCRIPTION
*
*       Local fn helper to mark compute minions free. The shire mask
*       provided by caller determine the minions to be made available.
*
*   INPUTS
*
*       shire_mask  Shire mask of compute minions to be marked free
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void kw_unreserve_kernel_shires(uint64_t shire_mask)
{
    /* Acquire the lock */
    acquire_local_spinlock(&KW_CB.resource_lock);

    /* Update the each shire status back to ready */
    for (uint64_t shire = 0; shire < NUM_SHIRES; shire++)
    {
        if (shire_mask & (1ULL << shire))
        {
            CW_Update_Shire_State(shire, CW_SHIRE_STATE_READY);
        }
    }

    /* Release the lock */
    release_local_spinlock(&KW_CB.resource_lock);
}

/************************************************************************
*
*   FUNCTION
*
*       KW_Dispatch_Kernel_Launch_Cmd
*
*   DESCRIPTION
*
*       KW Dispatch Kernel launch command
*
*   INPUTS
*
*       cmd         Kernel launch command
*       sqw_idx     Submission queue index
*       kw_idx      Pointer to get kernel work index (slot number)
*
*   OUTPUTS
*
*       int8_t      status success or error
*
***********************************************************************/
int8_t KW_Dispatch_Kernel_Launch_Cmd
    (struct device_ops_kernel_launch_cmd_t *cmd, uint8_t sqw_idx, uint8_t* kw_idx)
{
    kernel_instance_t *kernel = 0;
    int8_t status = KW_ERROR_KERNEL_INVALID_ADDRESS;
    uint8_t slot_index;

    /* Verify kernel start address */
    if(kw_check_address_bounds(cmd->code_start_address))
    {
        /* Kernel args are optional */
        if(cmd->pointer_to_args == 0)
        {
            status = STATUS_SUCCESS;
        }
        /* Verify kernel args address if provided */
        else if (kw_check_address_bounds(cmd->pointer_to_args))
        {
            status = STATUS_SUCCESS;
        }
    }

    if(status == STATUS_SUCCESS)
    {
        /* First we allocate resources needed for the kernel launch */
        /* Reserve a slot for the kernel */
        kernel = kw_reserve_kernel_slot(&slot_index);

        if(kernel)
        {
            /* Reserve compute shires needed for the requested
            kernel launch */
            status = kw_reserve_kernel_shires(cmd->shire_mask);

            if(status == STATUS_SUCCESS)
            {
                atomic_store_local_64(&kernel->kernel_shire_mask, cmd->shire_mask);
            }
            else
            {
                /* Make reserved kernel slot available again */
                kw_unreserve_kernel_slot(kernel);
                Log_Write(LOG_LEVEL_ERROR, "KW:ERROR:kernel shires unavailable\r\n");
            }
        }
        else
        {
            status = KW_ERROR_KERNEL_SLOT_UNAVAILABLE;
            Log_Write(LOG_LEVEL_ERROR, "KW:ERROR:kernel slot unavailable\r\n");
        }
    }

    if(status == STATUS_SUCCESS)
    {
        /* Populate the tag_id and sqw_idx for KW */
        atomic_store_local_16(&kernel->launch_tag_id, cmd->command_info.cmd_hdr.tag_id);
        atomic_store_local_16(&kernel->sqw_idx, sqw_idx);

        /* Populate the kernel launch params */
        mm_to_cm_message_kernel_launch_t launch_args;
        launch_args.header.id = MM_TO_CM_MESSAGE_ID_KERNEL_LAUNCH;
        launch_args.kw_base_id = (uint8_t)KW_MS_BASE_HART;
        launch_args.slot_index = slot_index;
        // SW-6502 - Pre launch flush are very expensive
        //launch_args.flags = KERNEL_LAUNCH_FLAGS_EVICT_L3_BEFORE_LAUNCH;
        launch_args.flags = 0x0;
        launch_args.code_start_address = cmd->code_start_address;
        launch_args.pointer_to_args = cmd->pointer_to_args;
        launch_args.shire_mask = cmd->shire_mask;

        /* Blocking call that blocks till all shires ack command */
        status = CM_Iface_Multicast_Send(launch_args.shire_mask,
                    (cm_iface_message_t*)&launch_args);

        if (status == STATUS_SUCCESS)
        {
            /* Set kernel start time */

            /* Update the each shire status to running*/
            for (uint64_t shire = 0; shire < NUM_SHIRES; shire++)
            {
                if (cmd->shire_mask & (1ULL << shire))
                {
                    CW_Update_Shire_State(shire, CW_SHIRE_STATE_RUNNING);
                }
            }
            *kw_idx = slot_index;
        }
        else
        {
            Log_Write(LOG_LEVEL_ERROR, "KW:ERROR:MM2CMLaunch:CommandMulticast:Failed\r\n");
            /* Broadcast message failed. Reclaim resources */
            kw_unreserve_kernel_shires(cmd->shire_mask);
            kw_unreserve_kernel_slot(kernel);
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       KW_Dispatch_Kernel_Abort_Cmd
*
*   DESCRIPTION
*
*       Dispatches a kernel abort command request to
*       relevant computer workers
*
*   INPUTS
*
*       cmd         Kernel abort command
*       sqw_idx     Submission queue index
*
*   OUTPUTS
*
*       int8_t      status success or error
*
***********************************************************************/
int8_t KW_Dispatch_Kernel_Abort_Cmd(struct device_ops_kernel_abort_cmd_t *cmd,
    uint8_t sqw_idx)
{
    int8_t status;
    uint8_t slot_index;
    cm_iface_message_t message = { 0 };
    struct device_ops_kernel_abort_rsp_t abort_rsp;

    /* Find the kernel associated with the given tag_id */
    status = kw_find_used_kernel_slot(cmd->kernel_launch_tag_id,
        &slot_index);

    if(status == STATUS_SUCCESS)
    {
        /* Update the kernel state to aborted */
        atomic_store_local_16(&KW_CB.kernels[slot_index].kernel_state,
            KERNEL_STATE_ABORTED_BY_HOST);

        /* Set the kernel abort message */
        message.header.id = MM_TO_CM_MESSAGE_ID_KERNEL_ABORT;

        /* Blocking call that blocks till all shires ack */
        status = CM_Iface_Multicast_Send(
            atomic_load_local_64(&KW_CB.kernels[slot_index].kernel_shire_mask),
            &message);

        /* Construct and transmit kernel abort response to host */
        abort_rsp.response_info.rsp_hdr.tag_id = cmd->command_info.cmd_hdr.tag_id;
        abort_rsp.response_info.rsp_hdr.msg_id =
            DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP;
        abort_rsp.response_info.rsp_hdr.size = sizeof(abort_rsp);

        /* Check the multicast send for errors */
        if(status == STATUS_SUCCESS)
        {
            abort_rsp.status = DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS;
        }
        else
        {
            Log_Write(LOG_LEVEL_ERROR, "KW:ERROR:MM2CMAbort:CommandMulticast:Failed\r\n");
            abort_rsp.status = DEV_OPS_API_KERNEL_ABORT_RESPONSE_ERROR;
        }

        /* Send kernel abort response to host */
        status = Host_Iface_CQ_Push_Cmd(0, &abort_rsp, sizeof(abort_rsp));

        if(status == STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_DEBUG, "KW:Pushed:KERNEL_ABORT_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                abort_rsp.response_info.rsp_hdr.tag_id);
        }
        else
        {
            Log_Write(LOG_LEVEL_ERROR, "KW:Push:KERNEL_ABORT_CMD_RSP:Failed\r\n");
        }

        /* Decrement commands count being processed by given SQW */
        SQW_Decrement_Command_Count(sqw_idx);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       KW_Init
*
*   DESCRIPTION
*
*       Initialize Kernel Worker
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
void KW_Init(void)
{
    /* Initialize the spinlock */
    init_local_spinlock(&KW_CB.resource_lock, 0);

    /* Mark all kernel slots - unused */
    for(int i = 0; i < MM_MAX_PARALLEL_KERNELS; i++)
    {
        /* Initialize FCC flags used by the kernel worker */
        atomic_store_local_8(&KW_CB.host2kw[i].fcc_id, FCC_0);
        global_fcc_init(&KW_CB.host2kw[i].fcc_flag);

        /* Initialize the tag IDs, sqw_idx and kernel state */
        atomic_store_local_64((uint64_t*)&KW_CB.kernels[i], 0U);

        /* Initialize shire_mask, wait and start cycle count */
        atomic_store_local_64(&KW_CB.kernels[i].kernel_shire_mask, 0U);
        atomic_store_local_64(&KW_CB.kernels[i].kw_cycles.raw_u64, 0U);
    }

    return;
}

/************************************************************************
*
*   FUNCTION
*
*       KW_Notify
*
*   DESCRIPTION
*
*       Notify KW Worker
*
*   INPUTS
*
*       kw_idx          ID of the kernel worker
*       cycle           Pointer containing 2 elements:
*                       -Wait Latency(time the command sits in Submission
*                         Queue)
*                       -Start cycles when Kernels are Launched on the
*                       Compute Minions
*       sw_timer_idx    Index of SW Timer used for timeout
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void KW_Notify(uint8_t kw_idx, const exec_cycles_t *cycle, uint8_t sw_timer_idx)
{
    uint32_t minion = (uint32_t)KW_WORKER_0 + (kw_idx / (2 / WORKER_HART_FACTOR));
    uint32_t thread = kw_idx % (2 / WORKER_HART_FACTOR);

    Log_Write(LOG_LEVEL_DEBUG, "Notifying:KW:minion=%d:thread=%d\r\n",
        minion, thread);

    atomic_store_local_64((void*)&KW_CB.kernels[kw_idx].kw_cycles,
                          cycle->raw_u64);
    atomic_store_local_8(&KW_CB.kernels[kw_idx].sw_timer_idx, sw_timer_idx);

    global_fcc_notify(atomic_load_local_8(&KW_CB.host2kw[kw_idx].fcc_id),
        &KW_CB.host2kw[kw_idx].fcc_flag, minion, thread);

    return;
}

/************************************************************************
*
*   FUNCTION
*
*       KW_Launch
*
*   DESCRIPTION
*
*       Launch a Kernel Worker on HART ID requested
*
*   INPUTS
*
*       uint32_t   HART ID to launch the Kernel Worker
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void KW_Launch(uint32_t hart_id, uint32_t kw_idx)
{
    uint64_t sip;
    cm_iface_message_t message;
    int8_t status;
    int8_t status_internal;
    uint16_t kernel_state;
    uint32_t done_cnt;
    uint32_t kernel_shires_count;
    uint64_t kernel_shire_mask;
    bool cw_exception;
    bool cw_error;
    /* Get the kernel instance */
    kernel_instance_t *const kernel = &KW_CB.kernels[kw_idx];

    Log_Write(LOG_LEVEL_DEBUG, "KW:H%d:IDX=%d\r\n", hart_id, kw_idx);

    while(1)
    {
        /* Wait on FCC notification from SQW Host Command Handler */
        global_fcc_wait(atomic_load_local_8(&KW_CB.host2kw[kw_idx].fcc_id),
            &KW_CB.host2kw[kw_idx].fcc_flag);

        Log_Write(LOG_LEVEL_DEBUG, "KW:Received:FCCEvent\r\n");

        /* Reset state */
        done_cnt = 0;
        cw_exception = false;
        cw_error = false;
        status_internal = STATUS_SUCCESS;

        /* Read the shire mask for the current kernel */
        kernel_shire_mask = atomic_load_local_64(&kernel->kernel_shire_mask);

        /* Calculate the number of shires involved in kernel launch */
        kernel_shires_count = (uint32_t)__builtin_popcountll(kernel_shire_mask);

        /* Process kernel command responses from CM, for all shires
        associated with the kernel launch */
        while(done_cnt < kernel_shires_count)
        {
            /* Wait for an interrupt */
            asm volatile("wfi");

            /* Read pending interrupts */
            SUPERVISOR_PENDING_INTERRUPTS(sip);

            /* We are only interesed in IPIs */
            if(!(sip & (1 << SUPERVISOR_SOFTWARE_INTERRUPT)))
            {
                continue;
            }

            /* Clear IPI pending interrupt */
            asm volatile("csrci sip, %0" : : "I"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

            /* Check the kernel_state is set to abort after timeout*/
            if(atomic_load_local_16(&kernel->kernel_state) == KERNEL_STATE_ABORTING)
            {
                Log_Write(LOG_LEVEL_ERROR, "Aborting:KW:kw_idx=%d\r\n", kw_idx);

                /* Break the loop waiting to complete the kernel as it is timeout */
                break;
            }

            /* Process all the available messages */
            while((done_cnt < kernel_shires_count) && (status_internal == STATUS_SUCCESS))
            {
                status = CM_Iface_Unicast_Receive(
                    CM_MM_KW_HART_UNICAST_BUFF_BASE_IDX + kw_idx, &message);

                if (status != STATUS_SUCCESS)
                {
                    /* No more pending messages left */
                    if ((status != CIRCBUFF_ERROR_BAD_LENGTH) &&
                        (status != CIRCBUFF_ERROR_EMPTY))
                    {
                        Log_Write(LOG_LEVEL_ERROR,
                            "KW:ERROR:CM_To_MM Receive failed. Status code: %d\r\n", status);
                    }
                    break;
                }

                /* Handle message from Compute Worker */
                switch (message.header.id)
                {
                    case CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE:
                    {
                        cm_to_mm_message_kernel_launch_completed_t *completed =
                            (cm_to_mm_message_kernel_launch_completed_t *)&message;

                        Log_Write(LOG_LEVEL_DEBUG,
                            "KW:from CW:CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE from S%d:Status:%d\r\n",
                            completed->shire_id, completed->status);

                        /* Check the completion status for any error
                        First time we get an error, set the error flag */
                        if((!cw_error) && (completed->status < KERNEL_COMPLETE_STATUS_SUCCESS))
                        {
                            cw_error = true;
                        }

                        /* Increase count of completed Shires */
                        done_cnt++;
                        break;
                    }
                    case CM_TO_MM_MESSAGE_ID_KERNEL_EXCEPTION:
                    {
                        cm_to_mm_message_exception_t *exception =
                            (cm_to_mm_message_exception_t *)&message;

                        /* TODO: SW-7528: We should receive a single exception message only */

                        Log_Write(LOG_LEVEL_DEBUG,
                            "KW:from CW:CM_TO_MM_MESSAGE_ID_KERNEL_EXCEPTION from S%" PRId32 "\r\n",
                            exception->shire_id);

                        /* First time we get an exception: abort kernel */
                        if(!cw_exception)
                        {
                            /* Only update the kernel state to exception and send abort
                            if it was not previously aborted by host */
                            if(atomic_load_local_16(&kernel->kernel_state) !=
                                KERNEL_STATE_ABORTED_BY_HOST)
                            {
                                /* Multicast abort to shires associated with current kernel slot
                                excluding the shire which took an exception */
                                message.header.id = MM_TO_CM_MESSAGE_ID_KERNEL_ABORT;

                                /* Blocking call (with timeout) that blocks till
                                all shires ack */
                                status_internal = CM_Iface_Multicast_Send(
                                    MASK_RESET_BIT(kernel_shire_mask, exception->shire_id),
                                    &message);
                            }

                            cw_exception = true;
                        }
                        /* Increase count of completed Shires */
                        done_cnt++;
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            }
        }

        /* Kernel run complete with host abort, exception or success.
        Prepare response and reclaim resources */

        struct device_ops_kernel_launch_rsp_t launch_rsp;

        /* Read the kernel state to detect abort by host */
        kernel_state = atomic_load_local_16(&kernel->kernel_state);

        /* Cancel the command timeout (if needed) */
        if(kernel_state != KERNEL_STATE_ABORTING)
        {
            /* Free the registered SW Timeout slot */
            SW_Timer_Cancel_Timeout(atomic_load_local_8(&kernel->sw_timer_idx));
        }

        /* NOTE: These checks below are in order of priority */
        if(kernel_state == KERNEL_STATE_ABORTED_BY_HOST)
        {
            /* Update the kernel launch response to indicate that it was aborted by host */
            launch_rsp.status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED;
        }
        else if(kernel_state == KERNEL_STATE_ABORTING)
        {
            /* Update the kernel launch response to indicate that it was aborted by host */
            launch_rsp.status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_TIMEOUT_HANG;
        }
        else if(cw_exception)
        {
            /* Exception was detected in kernel run, update response */
            launch_rsp.status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_EXCEPTION;
        }
        else if(cw_error)
        {
            /* Error was detected in kernel run, update response */
            launch_rsp.status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_ERROR;
        }
        else
        {
            /* Everything went normal, update response to kernel completed */
            launch_rsp.status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED;
        }

        /* Construct and transmit kernel launch response to host */
        launch_rsp.response_info.rsp_hdr.tag_id =
            atomic_load_local_16(&kernel->launch_tag_id);
        launch_rsp.response_info.rsp_hdr.msg_id =
            DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP;
        launch_rsp.response_info.rsp_hdr.size = sizeof(launch_rsp);
        launch_rsp.cmd_wait_time = atomic_load_local_32(&kernel->kw_cycles.wait_cycles);
        launch_rsp.cmd_execution_time = (uint32_t)PMC_GET_LATENCY(atomic_load_local_32(
                                        &kernel->kw_cycles.start_cycles)) & 0xFFFFFFFF;

        /* Send kernel launch response to host */
        status = Host_Iface_CQ_Push_Cmd(0, &launch_rsp, sizeof(launch_rsp));

        if(status == STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_DEBUG, "KW:Pushed:KERNEL_LAUNCH_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                launch_rsp.response_info.rsp_hdr.tag_id);
        }
        else
        {
            Log_Write(LOG_LEVEL_ERROR, "KW:Push:Failed\r\n");
        }

        /* Give back the reserved compute shires. */
        kw_unreserve_kernel_shires(kernel_shire_mask);

        /* Make reserved kernel slot available again */
        kw_unreserve_kernel_slot(kernel);

        /* Decrement commands count being processed by given SQW */
        SQW_Decrement_Command_Count(
            (uint8_t)atomic_load_local_16(&kernel->sqw_idx));
    } /* loop forever */

    /* will not return */
}

/************************************************************************
*
*   FUNCTION
*
*       KW_Set_Abort_Status
*
*   DESCRIPTION
*
*       Sets the status of a kernel to abort and notifies the KW
*
*   INPUTS
*
*       kw_idx    ID of the kernel worker
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void KW_Set_Abort_Status(uint8_t kw_idx)
{
    /* Free the registered SW Timeout slot */
    SW_Timer_Cancel_Timeout(atomic_load_local_8(&KW_CB.kernels[kw_idx].sw_timer_idx));

    atomic_store_local_16(&KW_CB.kernels[kw_idx].kernel_state, KERNEL_STATE_ABORTING);

    /* Trigger IPI to KW */
    syscall(SYSCALL_IPI_TRIGGER_INT,
        1ull << ((KW_BASE_HART_ID + (kw_idx * WORKER_HART_FACTOR)) % 64), MASTER_SHIRE, 0);
}
