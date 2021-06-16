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
    public and private interfaces. The kernel worker implements
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
#include    "device-common/atomic.h"
#include    "common_defs.h"
#include    "config/mm_config.h"
#include    "services/host_iface.h"
#include    "services/cm_iface.h"
#include    "services/sp_iface.h"
#include    "services/log.h"
#include    "services/sw_timer.h"
#include    "workers/cw.h"
#include    "workers/kw.h"
#include    "workers/sqw.h"
#include    "device-common/cacheops.h"
#include    "circbuff.h"
#include    "riscv_encoding.h"
#include    "sync.h"
#include    "device-common/utils.h"
#include    "vq.h"
#include    "syscall_internal.h"

/*! \typedef kernel_instance_t
    \brief Kernel Instance Control Block structure.
    Kernel instance maintains information related to
    a given kernel launch for the life time of kernel
    launch command.
*/
typedef struct kernel_instance_ {
    uint32_t kernel_state;
    tag_id_t launch_tag_id;
    uint16_t sqw_idx;
    uint8_t  sw_timer_idx;
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
    uint32_t            resource_timeout_flag[SQW_NUM];
} kw_cb_t;

/*! \struct kw_internal_status
    \brief Kernel Worker's internal status structure to
    track different types of errors.
*/
struct kw_internal_status{
    bool kernel_done;
    bool cw_exception;
    bool cw_error;
    int8_t status;
};

/*! \var kw_cb_t KW_CB
    \brief Global Kernel Worker Control Block
    \warning Not thread safe!
*/
static kw_cb_t KW_CB __attribute__((aligned(64))) = {0};

/* Local function prototypes */

static inline bool kw_check_address_bounds(uint64_t dev_address, bool is_optional)
{
    /* If the address check is optional,
    Address of zero means that do not verify the address (optional address) */
    if(is_optional && (dev_address == 0))
    {
        return true;
    }

    /* Verify the bounds */
    return ((dev_address >= HOST_MANAGED_DRAM_START) &&
            (dev_address < HOST_MANAGED_DRAM_END));
}

/************************************************************************
*
*   FUNCTION
*
*       kernel_acquire_resource_timeout_callback
*
*   DESCRIPTION
*
*       Callback for kernel free resource search timeout
*
*   INPUTS
*
*       sqw_idx    Submission queue index
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void kernel_acquire_resource_timeout_callback(uint8_t sqw_idx)
{
    /* Set the kernel slot timeout flag */
    atomic_store_local_32(&KW_CB.resource_timeout_flag[sqw_idx], 1U);
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
            if(atomic_load_local_32
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
*       sqw_idx             Submission queue index
*       slot_index          Index of available KW
*
*   OUTPUTS
*
*       kernel_instance_t*  Reference to kernel instance
*
***********************************************************************/
static kernel_instance_t* kw_reserve_kernel_slot(uint8_t sqw_idx, uint8_t *slot_index)
{
    kernel_instance_t* kernel = 0;
    int8_t sw_timer_idx;
    bool slot_reserved = false;

    /* Create timeout to wait for free kernel slot */
    sw_timer_idx = SW_Timer_Create_Timeout(&kernel_acquire_resource_timeout_callback, sqw_idx,
        KERNEL_SLOT_SEARCH_TIMEOUT);

    if(sw_timer_idx < 0)
    {
        Log_Write(LOG_LEVEL_ERROR,
            "KW: Unable to register kernel reserve slot timeout!\r\n");
        return kernel;
    }

    do
    {
        for(uint8_t i = 0; i < MM_MAX_PARALLEL_KERNELS; i++)
        {
            /* Find unused kernel slot and reserve it */
            if(atomic_compare_and_exchange_local_32
                (&KW_CB.kernels[i].kernel_state, KERNEL_STATE_UN_USED, KERNEL_STATE_IN_USE)
                == KERNEL_STATE_UN_USED)
            {
                kernel = &KW_CB.kernels[i];
                *slot_index = i;
                slot_reserved = true;
            }
        }
    } while (!slot_reserved &&
        (atomic_compare_and_exchange_local_32(&KW_CB.resource_timeout_flag[sqw_idx], 1, 0) == 0U));

    /* If timeout occurs then report this event to SP. */
    if(!slot_reserved)
    {
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_CM_RESERVE_SLOT_ERROR);
    }

    /* Free the registered SW Timeout slot */
    SW_Timer_Cancel_Timeout((uint8_t)sw_timer_idx);

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
    atomic_store_local_32(&kernel->kernel_state, KERNEL_STATE_UN_USED);
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
*       sqw_idx         Submission queue index
*       req_shire_mask  Shire mask of compute minions to be marked in-use
*
*   OUTPUTS
*
*       int8_t          status success or error
*
***********************************************************************/
static int8_t kw_reserve_kernel_shires(uint8_t sqw_idx, uint64_t req_shire_mask)
{
    int8_t status;
    int8_t sw_timer_idx;
    bool timeout = false;

    /* Acquire the lock */
    acquire_local_spinlock(&KW_CB.resource_lock);

    /* Create timeout for waiting for free shires for kernel */
    sw_timer_idx = SW_Timer_Create_Timeout(&kernel_acquire_resource_timeout_callback, sqw_idx,
        KERNEL_FREE_SHIRES_TIMEOUT);

    if(sw_timer_idx < 0)
    {
        Log_Write(LOG_LEVEL_ERROR,
            "KW: Unable to register timeout for waiting for shires to get free!\r\n");
        status = KW_ERROR_GENERAL;
    }
    else
    {
        /* Find and wait for the requested shire mask to get free */
        do
        {
            /* Check the required shires are available and ready */
            status = CW_Check_Shires_Available_And_Free(req_shire_mask);

            if(atomic_compare_and_exchange_local_32(&KW_CB.resource_timeout_flag[sqw_idx], 1, 0) == 0U)
            {
                timeout = true;
            }
        } while ((status != STATUS_SUCCESS) && (!timeout));

        /* Free the registered SW Timeout slot */
        SW_Timer_Cancel_Timeout((uint8_t)sw_timer_idx);

        if(status == STATUS_SUCCESS)
        {
            /* Mark the shires as busy */
            CW_Update_Shire_State(req_shire_mask, CW_SHIRE_STATE_BUSY);
        }
        else
        {
            status = KW_ERROR_KERNEL_SHIRES_NOT_READY;

            /* If timeout occurs then report this event to SP. */
            if(timeout)
            {
                SP_Iface_Report_Error(MM_RECOVERABLE, MM_CM_RESERVE_TIMEOUT_ERROR);
            }
            else
            {
                SP_Iface_Report_Error(MM_RECOVERABLE, MM_CM_RESERVE_SLOT_ERROR);
            }
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
    /* Update the each shire status back to ready */
    CW_Update_Shire_State(shire_mask, CW_SHIRE_STATE_FREE);
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

    /* Verify address bounds
       kernel start address (not optional)
       different addresses provided in the command could be optional address. */
    if(kw_check_address_bounds(cmd->code_start_address, false) &&
       kw_check_address_bounds(cmd->pointer_to_args, true) &&
       kw_check_address_bounds(cmd->exception_buffer, true) &&
       kw_check_address_bounds(cmd->kernel_trace_buffer,
        !(cmd->command_info.cmd_hdr.flags & CMD_HEADER_FLAG_KERNEL_TRACE_BUF)))
    {
        status = STATUS_SUCCESS;
    }

    if(status == STATUS_SUCCESS)
    {
        /* First we allocate resources needed for the kernel launch */
        /* Reserve a slot for the kernel */
        kernel = kw_reserve_kernel_slot(sqw_idx, &slot_index);

        if(kernel)
        {
            /* Reserve compute shires needed for the requested
            kernel launch */
            status = kw_reserve_kernel_shires(sqw_idx, cmd->shire_mask);

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

    /* Kernel arguments are optional (0 == optional) */
    if((status == STATUS_SUCCESS) && (cmd->pointer_to_args != 0))
    {
        /* Calculate the kernel arguments size */
        uint64_t args_size = (cmd->command_info.cmd_hdr.size - sizeof(*cmd));
        Log_Write(LOG_LEVEL_DEBUG, "KW:Kernel_launch_args_size: %ld\r\n", args_size);

        if((args_size > 0) && (args_size <= DEVICE_OPS_KERNEL_LAUNCH_ARGS_PAYLOAD_MAX))
        {
            /* Copy the kernel arguments from command buffer to the buffer address provided */
            ETSOC_MEM_COPY_AND_EVICT((void*)(uintptr_t)cmd->pointer_to_args,
                (void*)cmd->argument_payload, args_size, to_L3)
        }
        /* TODO: Enable this check once runtime has switched to using new kernel launch command
        else
        {
            status = KW_ERROR_KERNEL_INVLD_ARGS_SIZE
            Log_Write(LOG_LEVEL_ERROR, "KW:ERROR:kernel argument payload size invalid\r\n")
        }
        */
    }

    if(status == STATUS_SUCCESS)
    {
        /* Populate the tag_id and sqw_idx for KW */
        atomic_store_local_16(&kernel->launch_tag_id, cmd->command_info.cmd_hdr.tag_id);
        atomic_store_local_16(&kernel->sqw_idx, sqw_idx);

        /* Populate the kernel launch params */
        mm_to_cm_message_kernel_launch_t launch_args = {0};
        launch_args.header.id = MM_TO_CM_MESSAGE_ID_KERNEL_LAUNCH;
        launch_args.kw_base_id = (uint8_t)KW_MS_BASE_HART;
        launch_args.slot_index = slot_index;
        launch_args.code_start_address = cmd->code_start_address;
        launch_args.pointer_to_args = cmd->pointer_to_args;
        launch_args.shire_mask = cmd->shire_mask;
        launch_args.exception_buffer = cmd->exception_buffer;

        /* If the flag bit flush L3 is set */
        if(cmd->command_info.cmd_hdr.flags & CMD_HEADER_FLAG_KERNEL_FLUSH_L3)
        {
            launch_args.flags = KERNEL_LAUNCH_FLAGS_EVICT_L3_BEFORE_LAUNCH;
        }

        /* If the flag bit for U-mode trace buffer is set */
        if(cmd->command_info.cmd_hdr.flags & CMD_HEADER_FLAG_KERNEL_TRACE_BUF)
        {
            launch_args.trace_buffer = cmd->kernel_trace_buffer;
        }

        /* Blocking call that blocks till all shires ack command */
        status = CM_Iface_Multicast_Send(launch_args.shire_mask,
                    (cm_iface_message_t*)&launch_args);

        if (status == STATUS_SUCCESS)
        {
            *kw_idx = slot_index;
        }
        else
        {
            /* Broadcast message failed. Reclaim resources */
            kw_unreserve_kernel_shires(cmd->shire_mask);
            kw_unreserve_kernel_slot(kernel);

            Log_Write(LOG_LEVEL_ERROR, "KW:ERROR:MM2CMLaunch:CommandMulticast:Failed\r\n");
            SP_Iface_Report_Error(MM_RECOVERABLE, MM_MM2CM_CMD_ERROR);
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
        atomic_store_local_32(&KW_CB.kernels[slot_index].kernel_state,
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
        abort_rsp.response_info.rsp_hdr.size =
            sizeof(abort_rsp) - sizeof(struct cmn_header_t);

        /* Check the multicast send for errors */
        if(status == STATUS_SUCCESS)
        {
            abort_rsp.status = DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS;
        }
        else
        {
            abort_rsp.status = DEV_OPS_API_KERNEL_ABORT_RESPONSE_ERROR;
            SP_Iface_Report_Error(MM_RECOVERABLE, MM_MM2CM_CMD_ERROR);
            Log_Write(LOG_LEVEL_ERROR, "KW:ERROR:MM2CMAbort:CommandMulticast:Failed\r\n");
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
            SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
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

    atomic_store_local_64((void*)&KW_CB.kernels[kw_idx].kw_cycles.cmd_start_cycles,
                          cycle->cmd_start_cycles);

    atomic_store_local_64((void*)&KW_CB.kernels[kw_idx].kw_cycles.raw_u64,
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
*       kw_cm_to_mm_process_single_message
*
*   DESCRIPTION
*
*       Helper function to proccess a single CM-to-MM message.
*
*   INPUTS
*
*       message             CM to MM Message.
*       kernel_shire_mask   Shire mask of compute workers.
*       status_internal     Status control block to return status.
*       kernel              Kernel pointer.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static inline void kw_cm_to_mm_process_single_message(cm_iface_message_t *message,
    uint64_t kernel_shire_mask, struct kw_internal_status *status_internal, const kernel_instance_t *kernel)
{
    switch (message->header.id)
    {
        case CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE:
            /* Set the flag to indicate that kernel launch has completed */
            status_internal->kernel_done = true;

            const cm_to_mm_message_kernel_launch_completed_t *completed =
                (cm_to_mm_message_kernel_launch_completed_t *)message;

            Log_Write(LOG_LEVEL_DEBUG,
                "KW:from CW:CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE from S%d:Status:%d\r\n",
                completed->shire_id, completed->status);

            /* Check the completion status for any error
            First time we get an error, set the error flag */
            if((!status_internal->cw_error) && (completed->status < KERNEL_COMPLETE_STATUS_SUCCESS))
            {
                status_internal->cw_error = true;
            }
            break;

        case CM_TO_MM_MESSAGE_ID_KERNEL_EXCEPTION:
        {   //NOSONAR Is not feasible to break this code block into a new method.
            const cm_to_mm_message_exception_t *exception =
                (cm_to_mm_message_exception_t *)message;

            Log_Write(LOG_LEVEL_DEBUG,
                "KW:from CW:CM_TO_MM_MESSAGE_ID_KERNEL_EXCEPTION from S%" PRId32 "\r\n",
                exception->shire_id);

            /* First time we get an exception: abort kernel */
            if((atomic_load_local_32(&kernel->kernel_state) != KERNEL_STATE_ABORTED_BY_HOST) &&
                (!status_internal->cw_exception))
            {
                /* Only update the kernel state to exception and send abort
                   if it was not previously aborted by host.
                   Multicast abort to shires associated with current kernel slot
                   excluding the shire which took an exception */
                message->header.id = MM_TO_CM_MESSAGE_ID_KERNEL_ABORT;

                /* Blocking call (with timeout) that blocks till
                all shires ack */
                status_internal->status = CM_Iface_Multicast_Send(
                    MASK_RESET_BIT(kernel_shire_mask, exception->shire_id),
                    message);

                if(status_internal->status != STATUS_SUCCESS)
                {
                    SP_Iface_Report_Error(MM_RECOVERABLE, MM_MM2CM_CMD_ERROR);
                }
            }

            if (!status_internal->cw_exception)
            {
                status_internal->cw_exception = true;
            }
            break;
        }
        case CM_TO_MM_MESSAGE_ID_KERNEL_LAUNCH_ERROR:
            /* Multicast abort to shires associated with current kernel slot */
            message->header.id = MM_TO_CM_MESSAGE_ID_KERNEL_ABORT;

            const cm_to_mm_message_kernel_launch_error_t *error_mesg =
                (cm_to_mm_message_kernel_launch_error_t *)message;

            SP_Iface_Report_Error(MM_RECOVERABLE, MM_CM2MM_KERNEL_LAUNCH_ERROR);

            Log_Write(LOG_LEVEL_DEBUG,
                "KW:from CW:CM_TO_MM_MESSAGE_ID_KERNEL_LAUNCH_ERROR from H%" PRId64 "\r\n",
                error_mesg->hart_id);

            /* Fatal error received. Try to recover kernel shires by sending abort message.
               Blocking call (with timeout) that blocks till all shires ack */
            status_internal->status = CM_Iface_Multicast_Send(kernel_shire_mask, message);

            if(status_internal->status != STATUS_SUCCESS)
            {
                SP_Iface_Report_Error(MM_RECOVERABLE, MM_MM2CM_CMD_ERROR);
            }

            /* Set error and done flag */
            status_internal->cw_error = true;
            status_internal->kernel_done = true;
            break;

        default:
            break;
    }
}

/************************************************************************
*
*   FUNCTION
*
*       kw_get_kernel_launch_completion_status
*
*   DESCRIPTION
*
*       Helper function to get kernel launch completion status.
*       NOTE: In case of multiple error, it return high priority error only.
*
*   INPUTS
*
*       kernel_state    Launched kernel status
*       status_internal Internal status conataining all states codes.
*
*   OUTPUTS
*
*       uint32_t        Kernel launch completion status
*
***********************************************************************/
static inline uint32_t kw_get_kernel_launch_completion_status(uint32_t kernel_state,
    const struct kw_internal_status *status_internal)
{
    uint32_t status;

    /* These checks below are in order of priority */
    if(kernel_state == KERNEL_STATE_ABORTED_BY_HOST)
    {
        /* Update the kernel launch response to indicate that it was aborted by host */
        status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED;
    }
    else if(kernel_state == KERNEL_STATE_ABORTING)
    {
        /* Update the kernel launch response to indicate that it was aborted by host */
        status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_TIMEOUT_HANG;
    }
    else if(status_internal->cw_exception)
    {
        /* Exception was detected in kernel run, update response */
        status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_EXCEPTION;
    }
    else if(status_internal->cw_error)
    {
        /* Error was detected in kernel run, update response */
        status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_ERROR;
    }
    else
    {
        /* Everything went normal, update response to kernel completed */
        status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED;
    }

    return status;
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
    int8_t status_hang_abort;
    uint8_t local_sqw_idx;
    uint32_t kernel_state;
    uint64_t kernel_shire_mask;
    struct kw_internal_status status_internal;

    /* Get the kernel instance */
    kernel_instance_t *const kernel = &KW_CB.kernels[kw_idx];
    exec_cycles_t cycles;

    Log_Write(LOG_LEVEL_CRITICAL, "KW:H%d:IDX=%d\r\n", hart_id, kw_idx);

    while(1)
    {
        /* Wait on FCC notification from SQW Host Command Handler */
        global_fcc_wait(atomic_load_local_8(&KW_CB.host2kw[kw_idx].fcc_id),
            &KW_CB.host2kw[kw_idx].fcc_flag);

        Log_Write(LOG_LEVEL_DEBUG, "KW:Received:FCCEvent\r\n");

        /* Reset state */
        status_internal.kernel_done = false;
        status_internal.cw_exception = false;
        status_internal.cw_error = false;
        status_internal.status = STATUS_SUCCESS;
        status_hang_abort = STATUS_SUCCESS;

        /* Read the shire mask for the current kernel */
        kernel_shire_mask = atomic_load_local_64(&kernel->kernel_shire_mask);

        /* Process kernel command responses from CM, for all shires
        associated with the kernel launch */
        while(!status_internal.kernel_done && (status_internal.status == STATUS_SUCCESS))
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
            if(atomic_load_local_32(&kernel->kernel_state) == KERNEL_STATE_ABORTING)
            {
                Log_Write(LOG_LEVEL_ERROR, "Aborting:KW:kw_idx=%d\r\n", kw_idx);

                /* Multicast abort to shires associated with current kernel slot
                This abort should forcefully abort all the shires involved in
                kernel launch and if it timesout as well, do a reset of the shires. */
                message.header.id = MM_TO_CM_MESSAGE_ID_KERNEL_ABORT;

                /* Blocking call (with timeout) that blocks till all shires ack */
                status_hang_abort = CM_Iface_Multicast_Send(kernel_shire_mask, &message);

                /* Break the loop waiting to complete the kernel as it is timeout */
                break;
            }

            /* Receive the CM->MM message */
            status = CM_Iface_Unicast_Receive(
                CM_MM_KW_HART_UNICAST_BUFF_BASE_IDX + kw_idx, &message);

            if (status == STATUS_SUCCESS)
            {
                /* Handle message from Compute Worker */
                kw_cm_to_mm_process_single_message(&message, kernel_shire_mask, &status_internal, kernel);
            }
            else if ((status != CIRCBUFF_ERROR_BAD_LENGTH) &&
                     (status != CIRCBUFF_ERROR_EMPTY))
            {
                /* No more pending messages left */
                SP_Iface_Report_Error(MM_RECOVERABLE, MM_CM2MM_CMD_ERROR);
                Log_Write(LOG_LEVEL_ERROR,
                    "KW:ERROR:CM_To_MM Receive failed. Status code: %d\r\n", status);
            }
        }

        if(status_hang_abort != STATUS_SUCCESS)
        {
            /* TODO: SW-6569: Do the reset of the shires involved in kernel launch. */
            SP_Iface_Report_Error(MM_RECOVERABLE, MM_MM2CM_CMD_ERROR);
            Log_Write(LOG_LEVEL_ERROR,
                "KW:MM->CM:Abort hanged, doing reset of shires.status:%d\r\n", status_hang_abort);
        }

        /* Kernel run complete with host abort, exception or success.
        reclaim resources and Prepare response */

        struct device_ops_kernel_launch_rsp_t launch_rsp;

        /* Read the kernel state to detect abort by host */
        kernel_state = atomic_load_local_32(&kernel->kernel_state);

        /* Cancel the command timeout (if needed) */
        if(kernel_state != KERNEL_STATE_ABORTING)
        {
            /* Free the registered SW Timeout slot */
            SW_Timer_Cancel_Timeout(atomic_load_local_8(&kernel->sw_timer_idx));
        }

        /* Get completion status of kernel launch. */
        launch_rsp.status = kw_get_kernel_launch_completion_status(kernel_state, &status_internal);

        /* Read the execution cycles info */
        cycles.cmd_start_cycles = atomic_load_local_64(&kernel->kw_cycles.cmd_start_cycles);
        cycles.raw_u64 = atomic_load_local_64(&kernel->kw_cycles.raw_u64);

        /* Construct and transmit kernel launch response to host */
        launch_rsp.response_info.rsp_hdr.tag_id =
            atomic_load_local_16(&kernel->launch_tag_id);
        launch_rsp.response_info.rsp_hdr.msg_id =
            DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP;
        launch_rsp.response_info.rsp_hdr.size =
            sizeof(launch_rsp) - sizeof(struct cmn_header_t);
        launch_rsp.device_cmd_start_ts = cycles.cmd_start_cycles;
        launch_rsp.device_cmd_wait_dur = cycles.wait_cycles;
        launch_rsp.device_cmd_execute_dur = (PMC_GET_LATENCY(cycles.exec_start_cycles) & 0xFFFFFFFF);

        local_sqw_idx = (uint8_t)atomic_load_local_16(&kernel->sqw_idx);

        /* Give back the reserved compute shires. */
        kw_unreserve_kernel_shires(kernel_shire_mask);

        /* Make reserved kernel slot available again */
        kw_unreserve_kernel_slot(kernel);

        /* Send kernel launch response to host */
        status = Host_Iface_CQ_Push_Cmd(0, &launch_rsp, sizeof(launch_rsp));

        Log_Write(LOG_LEVEL_DEBUG, "KW:Pushed:KERNEL_LAUNCH_CMD_RSP:tag_id=%x->Host_CQ\r\n",
            launch_rsp.response_info.rsp_hdr.tag_id);

        if(status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR, "KW:Push:Failed\r\n");
            SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
        }

        /* Decrement commands count being processed by given SQW */
        SQW_Decrement_Command_Count(local_sqw_idx);

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

    atomic_store_local_32(&KW_CB.kernels[kw_idx].kernel_state, KERNEL_STATE_ABORTING);

    /* Trigger IPI to KW */
    syscall(SYSCALL_IPI_TRIGGER_INT,
        1ull << ((KW_BASE_HART_ID + (kw_idx * WORKER_HART_FACTOR)) % 64), MASTER_SHIRE, 0);
}
