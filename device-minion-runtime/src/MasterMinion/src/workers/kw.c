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
        KW_Fetch_Kernel_State
*/
/***********************************************************************/
#include    "atomic.h"
#include    "common_defs.h"
#include    "config/mm_config.h"
#include    "services/host_iface.h"
#include    "services/log1.h"
#include    "workers/cw.h"
#include    "workers/kw.h"
#include    "workers/sqw.h"
#include    "cacheops.h"
#include    "circbuff.h"
#include    "cm_to_mm_iface.h"
#include    "mm_to_cm_iface.h"
#include    "utils.h"
#include    "vq.h"

/*! \struct kernel_instance_t
    \brief Kernel Instance Control Block structure.
    Kernel instance maintains information related to
    a given kernel launch for the life time of kernel
    launch command.
*/
typedef struct kernel_instance_ {
    uint16_t slot_index;
    uint16_t tag_id;
    uint16_t sqw_idx;
    uint16_t kernel_state;
    exec_cycles_t kw_cycles;
    uint64_t kernel_shire_mask;
} kernel_instance_t;

/*! \struct kw_cb_t
    \brief Kernel Worker Control Block structure.
    Used to maintain kernel instance and related resources
    for the life time of MM runtime.
*/
typedef struct kw_cb_ {
    fcc_sync_cb_t       host2kw;
    spinlock_t          resource_lock;
    kernel_instance_t   kernels[MM_MAX_PARALLEL_KERNELS];
} kw_cb_t;

/*! \var kw_cb_t CQW_CB
    \brief Global Kernel Worker Control Block
    \warning Not thread safe!
*/
static kw_cb_t KW_CB __attribute__((aligned(64))) = {0};

extern spinlock_t Launch_Lock;

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
*       None
*
*   OUTPUTS
*
*       kernel_instance_t*  Reference to kernel instance
*
***********************************************************************/
static kernel_instance_t* kw_reserve_kernel_slot(void)
{
    kernel_instance_t* kernel = 0;

    /* Acquire the lock */
    acquire_local_spinlock(&KW_CB.resource_lock);

    for(int i = 0; i < MM_MAX_PARALLEL_KERNELS; i++)
    {
        if(atomic_load_local_16
            (&KW_CB.kernels[i].kernel_state) == KERNEL_STATE_UN_USED)
        {
            kernel = &KW_CB.kernels[i];
            atomic_store_local_16(&kernel->slot_index, (uint16_t)i);
            atomic_store_local_16(&kernel->kernel_state,
                KERNEL_STATE_IN_USE);
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
*       None
*
***********************************************************************/
static int8_t kw_reserve_kernel_shires(uint64_t req_shire_mask)
{
    int8_t status = STATUS_SUCCESS;

    /* Acquire the lock */
    acquire_local_spinlock(&KW_CB.resource_lock);

    /* Check the required shires are available and ready */
    status = CW_Check_Shire_Available_And_Ready(req_shire_mask);

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
*
*   OUTPUTS
*
*       uint8_t     status success or error
*
***********************************************************************/
int8_t KW_Dispatch_Kernel_Launch_Cmd
    (struct device_ops_kernel_launch_cmd_t *cmd, uint8_t sqw_idx)
{
    kernel_instance_t *kernel = 0;
    int8_t status;

    /* First we allocate resources needed for the kernel launch */
    /* Reserve a slot for the kernel */
    kernel = kw_reserve_kernel_slot();

    if(kernel)
    {
        /* Reserve compute shires needed for the requested
        kernel launch */
        status = kw_reserve_kernel_shires(cmd->shire_mask);

        if(status == STATUS_SUCCESS)
        {
            atomic_or_local_64
                (&kernel->kernel_shire_mask, cmd->shire_mask);
        }
        else
        {
            /* Make reserved kernel slot available again */
            kw_unreserve_kernel_slot(kernel);
            Log_Write(LOG_LEVEL_DEBUG, "%s%s%d",
                "KW:ERROR:kernel shires unavailable\r\n");
        }
    }
    else
    {
        status = KW_ERROR_KERNEL_SLOT_UNAVAILABLE;
        Log_Write(LOG_LEVEL_DEBUG, "%s%s%d",
            "KW:ERROR:kernel slot unavailable\r\n");
    }

    if(status == STATUS_SUCCESS)
    {
        /* Populate the tag_id and sqw_idx for KW */
        atomic_store_local_16(&kernel->tag_id,
            cmd->command_info.cmd_hdr.tag_id);
        atomic_store_local_16(&kernel->sqw_idx, sqw_idx);

        /* Populate the kernel launch params */
        mm_to_cm_message_kernel_launch_t launch_args;
        launch_args.header.id = MM_TO_CM_MESSAGE_ID_KERNEL_LAUNCH;
        launch_args.kw_base_id = (uint8_t)KW_BASE_HART_ID;
        /* TODO: This is used by the CMs to notify KW.
        Only base KW for now. */
        launch_args.slot_index = 0;
        launch_args.flags = KERNEL_LAUNCH_FLAGS_EVICT_L3_BEFORE_LAUNCH;
        launch_args.code_start_address = cmd->code_start_address;
        launch_args.pointer_to_args = cmd->pointer_to_args;
        launch_args.shire_mask = cmd->shire_mask;

        /* Blocking call that blocks till all shires ack command */
        status = (int8_t)MM_To_CM_Iface_Multicast_Send
                    (launch_args.shire_mask,
                    (cm_iface_message_t*)&launch_args);

        if (status == STATUS_SUCCESS)
        {
            /* Set kernel start time */

            /* Update the each shire status to running*/
            for (uint64_t shire = 0; shire < NUM_SHIRES; shire++)
            {
                if (atomic_load_local_64
                    (&kernel->kernel_shire_mask) & (1ULL << shire))
                {
                    CW_Update_Shire_State(shire, CW_SHIRE_STATE_RUNNING);
                }
            }
        }
        else
        {
            Log_Write(LOG_LEVEL_DEBUG, "%s%s%d",
                "KW:ERROR:MM2CMLaunch:CommandMulticast:Failed\r\n");
            /* Broadcast message failed. Reclaim resources */
            kw_unreserve_kernel_shires(kernel->kernel_shire_mask);
            kw_unreserve_kernel_slot(kernel);
        }
    }
    else
    {
        Log_Write(LOG_LEVEL_DEBUG, "%s%d%s",
            "KW:ERROR:MM2CMLaunch:ResourcesUnavailable:Failed:status:",
            status, "\r\n");
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       KW_Fetch_Kernel_State
*
*   DESCRIPTION
*
*       Fetch kernel state for the kernel ID requested
*
*   INPUTS
*
*       kernel_id   Kernel ID
*
*   OUTPUTS
*
*       uint8_t     status success or error
*
***********************************************************************/
uint8_t KW_Fetch_Kernel_State(uint8_t kernel_id)
{
    (void)kernel_id;

    /* TODO: Implement logic here to  fetch kernel state, the
    assumption is kernel id used will be the kernel slot id.
    CUrrently returning 0 since we support a single kernel only */

    /* TODO: The kernel launch response binding should be update
    to return to host a kernel ID, which will be used by host to
    query for kernel state */

    return 0;
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
*
*   OUTPUTS
*
*       uint8_t     status success or error
*
***********************************************************************/
int8_t KW_Dispatch_Kernel_Abort_Cmd
    (struct device_ops_kernel_abort_cmd_t *cmd, uint8_t sqw_idx)
{
    (void)cmd;
    (void)sqw_idx;
    int8_t status = 0;

    /* TODO: Implement logic here to  abort kernels running
    on shires associated with kernel ID identified by the command */

    /* TODO: The kernel launch response binding should be update
    to return to host a kernel ID, which will be used by host to
    query for kernel state */

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
    /* Initialize FCC flags used by
    the kernel worker */
    atomic_store_local_8(&KW_CB.host2kw.fcc_id, FCC_0);
    global_fcc_init(&KW_CB.host2kw.fcc_flag);

    /* Initialize the spinlock */
    init_local_spinlock(&KW_CB.resource_lock, 0);

    /* Mark all kernel slots - unused */
    for(int i = 0; i < MM_MAX_PARALLEL_KERNELS; i++)
    {
        /* Initialize id, tag_id, sqw_idx, kernel_state */
        atomic_store_local_64((uint64_t*)&KW_CB.kernels[i], 0U);
        atomic_store_local_64(&KW_CB.kernels[i].kernel_shire_mask, 0U);
        atomic_store_local_32(&KW_CB.kernels[i].kw_cycles.wait_cycles, 0U);
        atomic_store_local_32(&KW_CB.kernels[i].kw_cycles.start_cycles, 0U);
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
*       kw_idx    ID of the kernel worker
*       cycles    Pointer containing 2 elements:
*                 -Wait Latency(time the command sits in Submission
*                   Queue)
*                 -Start cycles when Kernels are Launched on the
*                 Compute Minions
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void KW_Notify(uint8_t kw_idx, const exec_cycles_t *cycle )
{
    uint32_t minion = (uint32_t)KW_WORKER_0 + (kw_idx / 2);
    uint32_t thread = kw_idx % 2;

    Log_Write(LOG_LEVEL_DEBUG,
        "%s%d%s%d%s", "Notifying:KW:minion=", minion, ":thread=",
        thread, "\r\n");

    /* Extract Wait cycles and start cycles */
    atomic_store_local_32(&KW_CB.kernels[kw_idx].kw_cycles.wait_cycles,
                          cycle->wait_cycles);
    atomic_store_local_32(&KW_CB.kernels[kw_idx].kw_cycles.start_cycles,
                          cycle->start_cycles);

    global_fcc_notify(atomic_load_local_8(&KW_CB.host2kw.fcc_id),
        &KW_CB.host2kw.fcc_flag, minion, thread);

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
    cm_iface_message_t message;
    int8_t status;
    uint32_t done_cnt = 0;
    uint32_t kernel_shires_count;
    bool cw_exception = false;
    kernel_instance_t *kernel;

    /* Release the launch lock to let other workers acquire it */
    release_local_spinlock(&Launch_Lock);

    Log_Write(LOG_LEVEL_DEBUG, "%s%d%s%d%s",
        "KW:HART=", hart_id, ":IDX=", kw_idx, "\r\n");

    /* Empty all FCCs */
    init_fcc(FCC_0);
    init_fcc(FCC_1);

    /* Disable global interrupts (sstatus.SIE = 0) to not trap to trap handler.
    But enable Supervisor Software Interrupts so that IPIs trap when in U-mode.
    RISC-V spec:
    "An interrupt i will be taken if bit i is set in both mip and mie,
    and if interrupts are globally enabled."*/
    asm volatile("csrci sstatus, 0x2\n");
    asm volatile("csrsi sie, 0x2\n");

    while(1)
    {
        /* Wait on FCC notification from SQW Host Command Handler */
        global_fcc_wait(atomic_load_local_8(&KW_CB.host2kw.fcc_id),
            &KW_CB.host2kw.fcc_flag);

        Log_Write(LOG_LEVEL_DEBUG, "%s",
            "KW:Received:FCCEvent\r\n");

        /* TODO: Set up watchdog to detect command timeout */

        /* Get the kernel instance */
        /* TODO: single kernel support for now */
        kernel = &KW_CB.kernels[0];

        /* Calculate the number of shires involved in kernel launch */
        kernel_shires_count = (uint32_t)
            __builtin_popcountll(atomic_load_local_64
            (&kernel->kernel_shire_mask));

        /* Process kernel command responses from CM, for all shires
        associated with the kernel launch */
        while(done_cnt < kernel_shires_count)
        {
            /* Wait for an IPI */
            asm volatile("wfi\n");
            asm volatile("csrci sip, 0x2");

            /* TODO: We are currently assuming single kernel support.
            We need to enable a comms interface between Host Command
            Handler and KW so kernel slot ID for the current
            FCC notification can be coveyed from Host Command handler
            to KW, this will be used to determine the CW unicast
            completion buffer offset, and other purposes */

            status = CM_To_MM_Iface_Unicast_Receive(
                CM_MM_KW_HART_UNICAST_BUFF_BASE_IDX +
                atomic_load_local_16(&kernel->slot_index), &message);

            if (status != STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG, "%s%s%d",
                    "KW:ERROR:CM_To_MM Receive failed. Status code: ",
                    status, "\r\n");
                break;
            }

            /* Handle message from Compute Worker */
            /* TODO: We should send shire ID as part of
            message payload so we can improve the way we
            aggregate and detect completion notifications
            from all shires for given kernel launch */
            switch (message.header.id)
            {
                case CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE:
                {
                    log_write(LOG_LEVEL_DEBUG,
                        "CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE\n");

                    /* Increase count of completed Shires */
                    done_cnt++;
                    break;
                }
                case CM_TO_MM_MESSAGE_ID_U_MODE_EXCEPTION:
                {
                    log_write(LOG_LEVEL_DEBUG,
                        "CM_TO_MM_MESSAGE_ID_U_MODE_EXCEPTION\n");

                    /* Even if there was an exception, that Shire will
                    still send a KERNEL_COMPLETE */
                    cw_exception = true;
                    break;
                }
                default:
                {
                    break;
                }
            }
        }

        /* Kernel run complete with exception or success. Prepare response
        and reclaim resources */

        /* Give back the reserved compute shires. */
        kw_unreserve_kernel_shires(kernel->kernel_shire_mask);

        /* Make reserved kernel slot available again */
        kw_unreserve_kernel_slot(kernel);

        /* Construct and transmit kernel launch response to host */
        struct device_ops_kernel_launch_rsp_t rsp;
        rsp.response_info.rsp_hdr.tag_id =
            atomic_load_local_16(&kernel->tag_id);
        rsp.response_info.rsp_hdr.msg_id =
            DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP;
        rsp.response_info.rsp_hdr.size = sizeof(rsp);
        rsp.cmd_wait_time = atomic_load_local_32(&kernel->kw_cycles.wait_cycles);
        rsp.cmd_execution_time = (uint32_t)PMC_GET_LATENCY(atomic_load_local_32(
                                           &kernel->kw_cycles.start_cycles)) & 0xFFFFFFFF;

        /* If an exception was detected */
        if(cw_exception)
        {
            /* TODO - Multicast abort to shires associated with
            current kernel slot,, see kernel.c */

            /* Set error status */
            rsp.status = DEV_OPS_API_KERNEL_LAUNCH_STATUS_RESULT_ERROR;
        }
        else
        {
            /* Set result OK response */
            rsp.status = DEV_OPS_API_KERNEL_LAUNCH_STATUS_RESULT_OK;
        }

        status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

        if(status == STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_DEBUG, "%s",
                "KW:Pushed:KERNEL_LAUNCH_CMD_RSP->Host_CQ \r\n");
        }
        else
        {
            Log_Write(LOG_LEVEL_DEBUG, "%s",
                "KW:Push:Failed\r\n");
        }

        /* Decrement commands count being processed by given SQW */
        SQW_Decrement_Command_Count(
            (uint8_t)atomic_load_local_16(&kernel->sqw_idx));
    }

    return;
}
