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
        KW_Abort_All_Dispatched_Kernels
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
#include    "services/trace.h"
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

/*! \def CM_KERNEL_LAUNCHED_FLAG
    \brief Macro that defines the flag for kernel launch status of CM side.
*/
#define CM_KERNEL_LAUNCHED_FLAG ((cm_kernel_launched_flag_t *)CM_KERNEL_LAUNCHED_FLAG_BASEADDR)

/*! \typedef kernel_instance_t
    \brief Kernel Instance Control Block structure.
    Kernel instance maintains information related to
    a given kernel launch for the life time of kernel
    launch command.
*/
typedef struct kernel_instance_ {
    uint32_t kernel_state;
    tag_id_t launch_tag_id;
    uint8_t sqw_idx;
    uint8_t pad;
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
    uint32_t            abort_wait_timeout_flag[SQW_NUM];
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
*       kernel_abort_wait_timeout_callback
*
*   DESCRIPTION
*
*       Callback for kernel abort wait timeout
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
static void kernel_abort_wait_timeout_callback(uint8_t sqw_idx)
{
    /* Set the kernel abort wait timeout flag */
    atomic_store_local_32(&KW_CB.abort_wait_timeout_flag[sqw_idx], 1U);
}

/************************************************************************
*
*   FUNCTION
*
*       kw_wait_for_kernel_launch_flag
*
*   DESCRIPTION
*
*       Local fn helper to wait for all the Compute Minions to complete
*       the launch of kernel.
*
*   INPUTS
*
*       sqw_idx        Submission queue index
*       slot_index     Index of available KW
*
*   OUTPUTS
*
*       int8_t         status success or error
*
***********************************************************************/
static int8_t kw_wait_for_kernel_launch_flag(uint8_t sqw_idx, uint8_t slot_index)
{
    int8_t status = STATUS_SUCCESS;
    int8_t sw_timer_idx;
    uint32_t timeout_flag;
    cm_kernel_launched_flag_t kernel_launched;

    /* Create timeout to wait for kernel launch completion flag from CM */
    sw_timer_idx = SW_Timer_Create_Timeout(&kernel_abort_wait_timeout_callback, sqw_idx,
        KERNEL_ABORT_WAIT_TIMEOUT);

    if(sw_timer_idx < 0)
    {
        Log_Write(LOG_LEVEL_ERROR,
            "KW: Unable to register kernel abort wait timeout!\r\n");
        status = KW_ERROR_SW_TIMER_REGISTER_FAIL;
    }
    else
    {
        do
        {
            /* Read the kernel launch flag from MM L2 SCP */
            kernel_launched.flag =
                atomic_load_global_32(&CM_KERNEL_LAUNCHED_FLAG[slot_index].flag);

            /* Read the timeout flag */
            timeout_flag = atomic_compare_and_exchange_local_32(
                &KW_CB.abort_wait_timeout_flag[sqw_idx], 1, 0);

        } while ((kernel_launched.flag == 0) && (timeout_flag == 0));

        /* Free the registered SW Timeout slot */
        SW_Timer_Cancel_Timeout((uint8_t)sw_timer_idx);

        /* Check for timeout */
        if(timeout_flag == 1)
        {
            status = KW_ERROR_TIMEDOUT_ABORT_WAIT;
        }
    }

    return status;
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
*       kernel              Pointer to kernel slot
*
*   OUTPUTS
*
*       int8_t              status success or error
*
***********************************************************************/
static int8_t kw_reserve_kernel_slot(uint8_t sqw_idx, uint8_t *slot_index,
    kernel_instance_t** kernel)
{
    int8_t status = STATUS_SUCCESS;
    sqw_state_e sqw_state;
    bool slot_reserved = false;

    do
    {
        for(uint8_t i = 0; i < MM_MAX_PARALLEL_KERNELS; i++)
        {
            /* Find unused kernel slot and reserve it */
            if(atomic_compare_and_exchange_local_32
                (&KW_CB.kernels[i].kernel_state, KERNEL_STATE_UN_USED, KERNEL_STATE_IN_USE)
                == KERNEL_STATE_UN_USED)
            {
                *kernel = &KW_CB.kernels[i];
                *slot_index = i;
                slot_reserved = true;
            }
        }
        /* Read the SQW state */
        sqw_state = SQW_Get_State(sqw_idx);
    } while (!slot_reserved && (sqw_state != SQW_STATE_ABORTED));

    /* Verify SQW state */
    if(sqw_state == SQW_STATE_ABORTED)
    {
        status = KW_ABORTED_KERNEL_SLOT_SEARCH;
        Log_Write(LOG_LEVEL_ERROR, "KW:ABORTED:kernel slot search\r\n");

        /* Unreserve the slot */
        if(slot_reserved)
        {
            atomic_store_local_32(&KW_CB.kernels[*slot_index].kernel_state, KERNEL_STATE_UN_USED);
        }
    }

    return status;
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
    sqw_state_e sqw_state;

    /* Verify the shire mask */
    if (req_shire_mask == 0)
    {
        return KW_ERROR_KERNEL_INVLD_SHIRE_MASK;
    }

    /* Acquire the lock */
    acquire_local_spinlock(&KW_CB.resource_lock);

    /* Find and wait for the requested shire mask to get free */
    do
    {
        /* Check the required shires are available and ready */
        status = CW_Check_Shires_Available_And_Free(req_shire_mask);

        /* Read the SQW state */
        sqw_state = SQW_Get_State(sqw_idx);
    } while ((status != STATUS_SUCCESS) && (status != CW_SHIRE_UNAVAILABLE)
        && (sqw_state != SQW_STATE_ABORTED));

    if((status == STATUS_SUCCESS) && (sqw_state != SQW_STATE_ABORTED))
    {
        /* Mark the shires as busy */
        CW_Update_Shire_State(req_shire_mask, CW_SHIRE_STATE_BUSY);

        /* Release the lock */
        release_local_spinlock(&KW_CB.resource_lock);
    }
    else
    {
        /* Release the lock */
        release_local_spinlock(&KW_CB.resource_lock);

        /* Verify SQW state */
        if(sqw_state == SQW_STATE_ABORTED)
        {
            status = KW_ABORTED_KERNEL_SHIRES_SEARCH;
            Log_Write(LOG_LEVEL_ERROR, "KW:ABORTED:kernel shires search\r\n");
        }
        else
        {
            status = KW_ERROR_KERNEL_SHIRES_NOT_READY;
            Log_Write(LOG_LEVEL_ERROR, "KW:ERROR:kernel shires unavailable\r\n");
            SP_Iface_Report_Error(MM_RECOVERABLE, MM_CM_RESERVE_SLOT_ERROR);
        }
    }

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
*       process_kernel_launch_cmd_payload
*
*   DESCRIPTION
*
*       This function processes the optional payload present in kernel
*       launch command
*
*   INPUTS
*
*       cmd         Kernel launch command
*
*   OUTPUTS
*
*       int8_t      status success or error
*
***********************************************************************/
static inline int8_t process_kernel_launch_cmd_payload(struct device_ops_kernel_launch_cmd_t *cmd)
{
    /* Calculate the kernel arguments size */
    uint64_t args_size = (cmd->command_info.cmd_hdr.size - sizeof(*cmd));
    uint8_t *payload = (uint8_t *)cmd->argument_payload;
    Log_Write(LOG_LEVEL_DEBUG, "KW: Kernel launch argument payload size: %ld\r\n", args_size);
    int8_t status = STATUS_SUCCESS;

    /* Check if Trace config are present in optional command payload. */
    if(cmd->command_info.cmd_hdr.flags & CMD_HEADER_FLAGS_COMPUTE_KERNEL_TRACE_ENABLE)
    {
        Log_Write(LOG_LEVEL_DEBUG, "KW:INFO: Trace Optional Payload present!\r\n");
        if((args_size >= sizeof(struct trace_init_info_t)) &&
            (args_size <= DEVICE_OPS_KERNEL_LAUNCH_ARGS_PAYLOAD_MAX))
        {
            struct trace_init_info_t *trace_config = (struct trace_init_info_t *)(uintptr_t)payload;

            if (IS_ALIGNED(trace_config->buffer, CACHE_LINE_SIZE) && IS_ALIGNED(trace_config->buffer_size, CACHE_LINE_SIZE))
            {
                /* Copy the Trace configs from command payload to provided address
                    NOTE: Trace configs are always present at the beginning of the payload
                    and its size is fixed.*/
                ETSOC_MEM_COPY_AND_EVICT((void*)(uintptr_t)CM_UMODE_TRACE_CFG_BASEADDR,
                    (void*)payload, sizeof(struct trace_init_info_t), to_L3)


                args_size -= sizeof(struct trace_init_info_t);
                payload += sizeof(struct trace_init_info_t);
            }
            else
            {
                status = KW_ERROR_KERNEL_INVALID_ADDRESS;
                Log_Write(LOG_LEVEL_ERROR, "KW:ERROR: Invalid UMode Trace Buffer\r\n");
            }
        }
        else
        {
            status = KW_ERROR_KERNEL_INVLD_ARGS_SIZE;
            Log_Write(LOG_LEVEL_ERROR, "KW:ERROR: Invalid Trace config payload size\r\n");
        }
    }

    /* Check if Kernel arguments are present in optional command payload. */
    if((status == STATUS_SUCCESS) && (cmd->pointer_to_args != 0))
    {
        Log_Write(LOG_LEVEL_DEBUG, "KW:INFO: Kernel Args Optional Payload present!\r\n");
        if((args_size > 0) && (args_size <= DEVICE_OPS_KERNEL_LAUNCH_ARGS_PAYLOAD_MAX))
        {
            /* Copy the kernel arguments from command payload to provided address
                NOTE: Kernel argument position depends upon other optional fields in payload,
                if there is no other optional payload then all data in payload is kernel args. */
            ETSOC_MEM_COPY_AND_EVICT((void*)(uintptr_t)cmd->pointer_to_args,
                (void*)payload, args_size, to_L3)
        }
        else
        {
            status = KW_ERROR_KERNEL_INVLD_ARGS_SIZE;
            Log_Write(LOG_LEVEL_ERROR, "KW:ERROR: Invalid Kernel argument payload size\r\n");
        }
    }

    return status;
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
       kw_check_address_bounds(cmd->exception_buffer, true))
    {
        /* First we allocate resources needed for the kernel launch */
        /* Reserve a slot for the kernel */
        status = kw_reserve_kernel_slot(sqw_idx, &slot_index, &kernel);

        if(status == STATUS_SUCCESS)
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
            }
        }
    }

    /* Kernel arguments are optional (0 == optional) */
    if(status == STATUS_SUCCESS)
    {
        status = process_kernel_launch_cmd_payload(cmd);
    }

    if(status == STATUS_SUCCESS)
    {
        /* Populate the tag_id and sqw_idx for KW */
        atomic_store_local_16(&kernel->launch_tag_id, cmd->command_info.cmd_hdr.tag_id);
        atomic_store_local_8(&kernel->sqw_idx, sqw_idx);

        /* Populate the kernel launch params */
        mm_to_cm_message_kernel_launch_t launch_args = {0};
        launch_args.header.id = MM_TO_CM_MESSAGE_ID_KERNEL_LAUNCH;
        launch_args.kernel.kw_base_id = (uint8_t)KW_MS_BASE_HART;
        launch_args.kernel.slot_index = slot_index;
        launch_args.kernel.code_start_address = cmd->code_start_address;
        launch_args.kernel.pointer_to_args = cmd->pointer_to_args;
        launch_args.kernel.shire_mask = cmd->shire_mask;
        launch_args.kernel.exception_buffer = cmd->exception_buffer;

        /* If the flag bit flush L3 is set */
        if(cmd->command_info.cmd_hdr.flags & CMD_HEADER_FLAG_KERNEL_FLUSH_L3)
        {
            launch_args.kernel.flags = KERNEL_LAUNCH_FLAGS_EVICT_L3_BEFORE_LAUNCH;
        }

        if(cmd->command_info.cmd_hdr.flags & CMD_HEADER_FLAGS_COMPUTE_KERNEL_TRACE_ENABLE)
        {
            launch_args.kernel.flags |= KERNEL_LAUNCH_FLAGS_COMPUTE_KERNEL_TRACE_ENABLE;
        }

        /* Reset the L2 SCP kernel launched flag for the acquired kernel worker slot */
        atomic_store_global_32(&CM_KERNEL_LAUNCHED_FLAG[slot_index].flag, 0);

        TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD, sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_EXECUTING)

        /* Blocking call that blocks till all shires ack command */
        status = CM_Iface_Multicast_Send(launch_args.kernel.shire_mask,
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
        /* Wait until all the harts on CM side have launched kernel */
        status = kw_wait_for_kernel_launch_flag(sqw_idx, slot_index);

        if(status == STATUS_SUCCESS)
        {
            /* Update the kernel state to aborted */
            atomic_store_local_32(&KW_CB.kernels[slot_index].kernel_state,
                KERNEL_STATE_ABORTED_BY_HOST);

            /* Set the kernel abort message */
            message.header.id = MM_TO_CM_MESSAGE_ID_KERNEL_ABORT;

            /* Command status trace log */
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_EXECUTING)

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
        else
        {
            /* Report error to SP */
            SP_Iface_Report_Error(MM_RECOVERABLE, MM_CM_KERNEL_ABORT_TIMEOUT_ERROR);
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       KW_Abort_All_Dispatched_Kernels
*
*   DESCRIPTION
*
*       Blocking function call that sets the status of each kernel to
*       abort and notifies the KW
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
void KW_Abort_All_Dispatched_Kernels(uint8_t sqw_idx)
{
    /* Traverse all kernel slots and abort them */
    for(uint8_t kw_idx = 0; kw_idx < MM_MAX_PARALLEL_KERNELS; kw_idx++)
    {
        /* Check if this kernel slot is used by the given sqw_idx
        and kernel slot is in use, then abort it */
        if((atomic_load_local_8(&KW_CB.kernels[kw_idx].sqw_idx) == sqw_idx) &&
            (atomic_compare_and_exchange_local_32(&KW_CB.kernels[kw_idx].kernel_state,
            KERNEL_STATE_IN_USE, KERNEL_STATE_ABORTING) == KERNEL_STATE_IN_USE))
        {
            /* Trigger IPI to KW */
            syscall(SYSCALL_IPI_TRIGGER_INT,
                1ULL << ((KW_BASE_HART_ID + (kw_idx * HARTS_PER_MINION)) % 64),
                MASTER_SHIRE, 0);

            /* Spin-wait if the KW state is aborting */
            do
            {
                asm volatile("fence\n" ::: "memory");
            } while (atomic_load_local_32(&KW_CB.kernels[kw_idx].kernel_state) == KERNEL_STATE_ABORTING);
        }
    }
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
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void KW_Notify(uint8_t kw_idx, const exec_cycles_t *cycle)
{
    uint32_t minion = KW_WORKER_0 + kw_idx;

    Log_Write(LOG_LEVEL_DEBUG, "Notifying:KW:minion=%d:thread=%d\r\n",
        minion, KW_THREAD_ID);

    atomic_store_local_64((void*)&KW_CB.kernels[kw_idx].kw_cycles.cmd_start_cycles,
                          cycle->cmd_start_cycles);

    atomic_store_local_64((void*)&KW_CB.kernels[kw_idx].kw_cycles.raw_u64,
                          cycle->raw_u64);

    global_fcc_notify(atomic_load_local_8(&KW_CB.host2kw[kw_idx].fcc_id),
        &KW_CB.host2kw[kw_idx].fcc_flag, minion, KW_THREAD_ID);

    return;
}

/************************************************************************
*
*   FUNCTION
*
*       kw_cm_to_mm_kernel_force_abort
*
*   DESCRIPTION
*
*       Local fn helper to abort a kernel currently running on CMs.
*
*   INPUTS
*
*       kernel_shire_mask  Kernel shire mask
*       reset_on_failure   Reset CMs on abort multicast failure
*
*   OUTPUTS
*
*       int8_t             status success or error
*
***********************************************************************/
static inline int8_t kw_cm_to_mm_kernel_force_abort(uint64_t kernel_shire_mask,
    bool reset_on_failure)
{
    cm_iface_message_t abort_msg = { .header.id = MM_TO_CM_MESSAGE_ID_KERNEL_ABORT };
    int8_t status;

    /* Blocking call (with timeout) that blocks till all shires ack */
    status = CM_Iface_Multicast_Send(kernel_shire_mask, &abort_msg);

    /* Verify that there is no abort hang situation recovery failure */
    if(status != STATUS_SUCCESS)
    {
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_MM2CM_CMD_ERROR);

        if(reset_on_failure)
        {
            Log_Write(LOG_LEVEL_ERROR,
                "KW:MM->CM:Abort hanged, doing reset of shires.status:%d\r\n", status);

            /* Get the mask for available shires in the device */
            uint64_t available_shires = CW_Get_Physically_Enabled_Shires();
            int8_t reset_status;

            /* Send cmd to SP to reset all the available shires */
            /* TODO: We are sending MM shire to reset as well, hence all MM Minions
            will reset. This needs to be fixed on SP side. SP needs to check for
            MM shire and only reset sync Minions. */
            reset_status = SP_Iface_Reset_Minion(available_shires);

            if(reset_status == STATUS_SUCCESS)
            {
                /* Wait for all shires to boot up */
                reset_status = CW_Wait_For_Compute_Minions_Boot(available_shires);
            }

            if(reset_status != STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "KW: Unable to reset all the available shires in device (status: %d)\r\n",
                    reset_status);
            }
        }
    }

    return status;
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
*       kw_id               Index of kernel worker.
*       kernel_shire_mask   Shire mask of compute workers.
*       status_internal     Status control block to return status.
*       kernel              Kernel pointer.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static inline void kw_cm_to_mm_process_single_message(uint32_t kw_idx, uint64_t kernel_shire_mask,
    struct kw_internal_status *status_internal, const kernel_instance_t *kernel)
{
    cm_iface_message_t message;
    int8_t status;
    (void)kernel;

    /* In case of kernel exception response, we need to acquire the CM->MM unicast
    lock to make sure that the access to buffers is serialized */
    if (status_internal->cw_exception)
    {
        /* Acquire the unicast lock */
        CM_Iface_Unicast_Acquire_Lock(CM_MM_KW_HART_UNICAST_BUFF_BASE_IDX + kw_idx);

        /* Receive the CM->MM message */
        status = CM_Iface_Unicast_Receive(
            CM_MM_KW_HART_UNICAST_BUFF_BASE_IDX + kw_idx, &message);

        /* Release the unicast lock */
        CM_Iface_Unicast_Release_Lock(CM_MM_KW_HART_UNICAST_BUFF_BASE_IDX + kw_idx);
    }
    else
    {
        /* Receive the CM->MM message */
        status = CM_Iface_Unicast_Receive(
            CM_MM_KW_HART_UNICAST_BUFF_BASE_IDX + kw_idx, &message);
    }

    if ((status != STATUS_SUCCESS) && (status != CIRCBUFF_ERROR_BAD_LENGTH) &&
        (status != CIRCBUFF_ERROR_EMPTY))
    {
        /* No more pending messages left */
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_CM2MM_CMD_ERROR);
        Log_Write(LOG_LEVEL_ERROR,
            "KW:ERROR:CM_To_MM Receive failed. Status code: %d\r\n", status);

        return;
    }

    switch (message.header.id)
    {
        case CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE:
            /* Set the flag to indicate that kernel launch has completed */
            status_internal->kernel_done = true;

            const cm_to_mm_message_kernel_launch_completed_t *completed =
                (cm_to_mm_message_kernel_launch_completed_t *)&message;

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
        {
            const cm_to_mm_message_exception_t *exception =
                (cm_to_mm_message_exception_t *)&message;

            Log_Write(LOG_LEVEL_DEBUG,
                "KW:from CW:CM_TO_MM_MESSAGE_ID_KERNEL_EXCEPTION from S%" PRId32 "\r\n",
                exception->shire_id);

            if (!status_internal->cw_exception)
            {
                status_internal->cw_exception = true;
            }
            break;
        }
        case CM_TO_MM_MESSAGE_ID_KERNEL_LAUNCH_ERROR:
            /* Report error to SP */
            SP_Iface_Report_Error(MM_RECOVERABLE, MM_CM2MM_KERNEL_LAUNCH_ERROR);

            const cm_to_mm_message_kernel_launch_error_t *error_mesg =
                (cm_to_mm_message_kernel_launch_error_t *)&message;

            Log_Write(LOG_LEVEL_DEBUG,
                "KW:from CW:CM_TO_MM_MESSAGE_ID_KERNEL_LAUNCH_ERROR from H%" PRId64 "\r\n",
                error_mesg->hart_id);

            /* Fatal error received. Try to recover kernel shires by sending abort message */
            status_internal->status = kw_cm_to_mm_kernel_force_abort(kernel_shire_mask, false);

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
    if((kernel_state == KERNEL_STATE_ABORTED_BY_HOST) ||
        (kernel_state == KERNEL_STATE_ABORTING))
    {
        /* Update the kernel launch response to indicate that it was aborted by host */
        status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED;

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
    bool timeout_abort_serviced;
    int8_t status;
    uint8_t local_sqw_idx;
    uint32_t kernel_state;
    uint64_t kernel_shire_mask;
    struct kw_internal_status status_internal;

    /* Get the kernel instance */
    kernel_instance_t *const kernel = &KW_CB.kernels[kw_idx];
    exec_cycles_t cycles;

    Log_Write(LOG_LEVEL_INFO, "KW:H%d:IDX=%d\r\n", hart_id, kw_idx);

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
        timeout_abort_serviced = false;
        status_internal.status = STATUS_SUCCESS;

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

            /* Check the kernel_state is set to abort after timeout */
            if((!timeout_abort_serviced) &&
                (atomic_load_local_32(&kernel->kernel_state) == KERNEL_STATE_ABORTING))
            {
                timeout_abort_serviced = true;
                Log_Write(LOG_LEVEL_ERROR, "KW:Aborting:kw_idx=%d\r\n", kw_idx);

                /* Multicast abort to shires associated with current kernel slot
                This abort should forcefully abort all the shires involved in
                kernel launch and if it times out as well, do a reset of the shires. */
                status_internal.status = kw_cm_to_mm_kernel_force_abort(kernel_shire_mask, true);
            }
            else
            {
                /* Handle message from Compute Worker */
                kw_cm_to_mm_process_single_message(kw_idx, kernel_shire_mask,
                    &status_internal, kernel);
            }
        }

        /* Kernel run complete with host abort, exception or success.
        reclaim resources and Prepare response */

        struct device_ops_kernel_launch_rsp_t launch_rsp = {0};

        /* Read the kernel state to detect abort by host */
        kernel_state = atomic_load_local_32(&kernel->kernel_state);

        /* Read the execution cycles info */
        cycles.cmd_start_cycles = atomic_load_local_64(&kernel->kw_cycles.cmd_start_cycles);
        cycles.raw_u64 = atomic_load_local_64(&kernel->kw_cycles.raw_u64);

        /* Construct and transmit kernel launch response to host */
        launch_rsp.response_info.rsp_hdr.tag_id =
            atomic_load_local_16(&kernel->launch_tag_id);
        launch_rsp.response_info.rsp_hdr.msg_id =
            DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP;
        launch_rsp.device_cmd_start_ts = cycles.cmd_start_cycles;
        launch_rsp.device_cmd_wait_dur = cycles.wait_cycles;
        launch_rsp.device_cmd_execute_dur = (PMC_GET_LATENCY(cycles.exec_start_cycles) & 0xFFFFFFFF);

        local_sqw_idx = atomic_load_local_8(&kernel->sqw_idx);

        /* Get completion status of kernel launch. */
        launch_rsp.status = kw_get_kernel_launch_completion_status(kernel_state, &status_internal);

        /* Give back the reserved compute shires. */
        kw_unreserve_kernel_shires(kernel_shire_mask);

        /* Make reserved kernel slot available again */
        kw_unreserve_kernel_slot(kernel);

#if TEST_FRAMEWORK
        /* For SP2MM command response, we need to provide the total size = header + payload */
        launch_rsp.response_info.rsp_hdr.size = sizeof(launch_rsp);
        /* Send kernel launch response to SP */
        status = SP_Iface_Push_Rsp_To_SP2MM_CQ(&launch_rsp, sizeof(launch_rsp));
#else
        launch_rsp.response_info.rsp_hdr.size =
            sizeof(launch_rsp) - sizeof(struct cmn_header_t);
        /* Send kernel launch response to host */
        status = Host_Iface_CQ_Push_Cmd(0, &launch_rsp, sizeof(launch_rsp));
#endif

        if(status == STATUS_SUCCESS)
        {
            /* Log to command status to trace */
            if(launch_rsp.status == DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED)
            {
                TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD,
                    local_sqw_idx, launch_rsp.response_info.rsp_hdr.tag_id, CMD_STATUS_SUCCEEDED);
            }
            else if(launch_rsp.status == DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED)
            {
                TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD,
                    local_sqw_idx, launch_rsp.response_info.rsp_hdr.tag_id, CMD_STATUS_ABORTED);
            }
            else
            {
                TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD,
                    local_sqw_idx, launch_rsp.response_info.rsp_hdr.tag_id, CMD_STATUS_FAILED);
            }

            Log_Write(LOG_LEVEL_DEBUG, "KW:CQ_Push:KERNEL_LAUNCH_CMD_RSP:tag_id=%x\r\n",
                launch_rsp.response_info.rsp_hdr.tag_id);
        }
        else
        {
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD,
                local_sqw_idx, launch_rsp.response_info.rsp_hdr.tag_id, CMD_STATUS_FAILED);

            Log_Write(LOG_LEVEL_ERROR, "KW:CQ_Push:Failed\r\n");
            SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
        }

#if !TEST_FRAMEWORK
        /* Decrement commands count being processed by given SQW */
        SQW_Decrement_Command_Count(local_sqw_idx);
#else
        (void)local_sqw_idx;
#endif
    } /* loop forever */

    /* will not return */
}
