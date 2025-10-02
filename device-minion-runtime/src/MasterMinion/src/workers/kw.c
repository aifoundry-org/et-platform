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
        KW_Get_Average_Exec_Cycles
*/
/***********************************************************************/
/* mm_rt_svcs */
#include <etsoc/isa/atomic.h>
#include <etsoc/common/common_defs.h>
#include <etsoc/isa/cacheops.h>
#include <etsoc/isa/riscv_encoding.h>
#include <etsoc/isa/sync.h>
#include <etsoc/isa/syscall.h>
#include <system/abi.h>
#include <transports/circbuff/circbuff.h>
#include <transports/vq/vq.h>

/* mm_rt_helpers */
#include "error_codes.h"
#include "syscall_internal.h"
#include "common_utils.h"

/* mm specific headers */
#include "config/mm_config.h"
#include "services/host_cmd_hdlr.h"
#include "services/host_iface.h"
#include "services/cm_iface.h"
#include "services/sp_iface.h"
#include "services/log.h"
#include "services/sw_timer.h"
#include "services/trace.h"
#include "workers/cw.h"
#include "workers/kw.h"
#include "workers/sqw.h"
#include "workers/statw.h"

/*! \def CM_KERNEL_LAUNCHED_FLAG
    \brief Macro that defines the flag for kernel launch status of CM side.
*/
#define CM_KERNEL_LAUNCHED_FLAG ((cm_kernel_launched_flag_t *)CM_KERNEL_LAUNCHED_FLAG_BASEADDR)

/*! \def KW_WAIT_AND_CLEAR_SW_INTERRUPT(enable)
    \brief Macro used to wait and clear IPIs.
*/
#define KW_WAIT_AND_CLEAR_SW_INTERRUPT(enable)                                         \
    {                                                                                  \
        if (enable)                                                                    \
        {                                                                              \
            uint64_t sip;                                                              \
                                                                                       \
            /* Wait for an interrupt */                                                \
            asm volatile("wfi");                                                       \
                                                                                       \
            /* Read pending interrupts */                                              \
            SUPERVISOR_PENDING_INTERRUPTS(sip);                                        \
                                                                                       \
            /* We are only interesed in IPIs */                                        \
            if (!(sip & (1 << SUPERVISOR_SOFTWARE_INTERRUPT)))                         \
            {                                                                          \
                continue;                                                              \
            }                                                                          \
                                                                                       \
            /* Clear IPI pending interrupt */                                          \
            asm volatile("csrci sip, %0" : : "I"(1 << SUPERVISOR_SOFTWARE_INTERRUPT)); \
        }                                                                              \
    }

/*! \def KERNEL_SAVE_UMODE_TRACE_PTR(kernel, cmd)
    \brief Macro used to save user mode trace pointer in KW CB.
*/
#define KW_SAVE_UMODE_TRACE_PTR(kernel, cmd)                                                   \
    {                                                                                          \
        if (cmd->command_info.cmd_hdr.flags & CMD_FLAGS_COMPUTE_KERNEL_TRACE_ENABLE)           \
        {                                                                                      \
            atomic_store_local_64(&kernel->umode_trace_buffer_ptr,                             \
                ((struct trace_init_info_t *)(uintptr_t)CM_UMODE_TRACE_CFG_BASEADDR)->buffer); \
        }                                                                                      \
        else                                                                                   \
        {                                                                                      \
            atomic_store_local_64(&kernel->umode_trace_buffer_ptr, 0);                         \
        }                                                                                      \
    }

#define KW_REGISTER_CM_ABORT_TIMER(kw_abort_timer, kw_idx, status)                                    \
    if (status == STATUS_SUCCESS)                                                                     \
    {                                                                                                 \
        /* Create timeout to wait for kernel complete msg from CMs after KW abort */                  \
        kw_abort_timer = SW_Timer_Create_Timeout(                                                     \
            &kw_cm_abort_wait_timeout_callback, (uint8_t)kw_idx, KERNEL_CM_ABORT_WAIT_TIMEOUT);       \
                                                                                                      \
        if (kw_abort_timer < 0)                                                                       \
        {                                                                                             \
            Log_Write(LOG_LEVEL_WARNING,                                                              \
                "KW:Unable to register KW CM abort timeout! It may not recover in case of hang\r\n"); \
        }                                                                                             \
    }

#define KW_INIT_KERNEL_ENV_SHIRE_MASK(slot_index, mask)                                     \
    {                                                                                       \
        kernel_environment_t *kernel_env =                                                  \
            (kernel_environment_t *)(CM_KERNEL_ENVS_BASEADDR +                              \
                                     (uint32_t)slot_index * KERNEL_ENV_SIZE);               \
                                                                                            \
        kernel_env->shire_mask = mask;                                                      \
                                                                                            \
        /* Evict the data to L2 SCP */                                                      \
        ETSOC_MEM_EVICT((void *)(uintptr_t)kernel_env, sizeof(kernel_environment_t), to_L2) \
    }

#define KW_COPY_CM_UMODE_TRACE_CFG_OPTIONALLY(slot, kernel_cmd)                                    \
    {                                                                                              \
        /* TODO: SW-16477: make the CM U-mode trace cfg per slot */                                \
        if (kernel_cmd->command_info.cmd_hdr.flags & CMD_FLAGS_COMPUTE_KERNEL_TRACE_ENABLE)        \
        {                                                                                          \
            /* Copy the Trace config from command payload to provided address */                   \
            /* NOTE: Trace config is always present at the begining of payload with fixed size. */ \
            ETSOC_MEM_COPY_AND_EVICT((void *)(uintptr_t)CM_UMODE_TRACE_CFG_BASEADDR,               \
                (void *)(uintptr_t)kernel_cmd->argument_payload, sizeof(struct trace_init_info_t), \
                to_L3)                                                                             \
        }                                                                                          \
    }

/*! \typedef kernel_instance_t
    \brief Kernel Instance Control Block structure.
    Kernel instance maintains information related to
    a given kernel launch for the life time of kernel
    launch command.
*/
typedef struct kernel_instance_ {
    execution_cycles_t kw_cycles;
    uint64_t kernel_exec_cycles; /* Total cycles consumed while executing kernels.
                                     Stat worker will reset this upon reading. */
    uint64_t kernel_shire_mask;
    uint64_t umode_exception_buffer_ptr;
    uint64_t umode_trace_buffer_ptr;
    uint32_t kernel_state;
    tag_id_t launch_tag_id;
    uint8_t sqw_idx;
    uint8_t cm_abort_wait_timeout_flag;
} kernel_instance_t;

/*! \typedef kw_cb_t
    \brief Kernel Worker Control Block structure.
    Used to maintain kernel instance and related resources
    for the life time of MM runtime.
*/
typedef struct kw_cb_ {
    uint64_t host_managed_dram_end;
    fcc_sync_cb_t host2kw[MM_MAX_PARALLEL_KERNELS];
    spinlock_t resource_lock;
    kernel_instance_t kernels[MM_MAX_PARALLEL_KERNELS];
    uint32_t launch_wait_timeout_flag[SQW_NUM];
} kw_cb_t;

/*! \struct kw_internal_status
    \brief Kernel Worker's internal status structure to
    track different types of errors.
*/
struct kw_internal_status {
    uint64_t cm_error_shire_mask;
    int32_t status;
    bool kernel_done;
    bool cw_exception;
    bool cw_error;
};

/*! \var kw_cb_t KW_CB
    \brief Global Kernel Worker Control Block
    \warning Not thread safe!
*/
static kw_cb_t KW_CB __attribute__((aligned(64))) = { 0 };

/* Local function prototypes */

static inline bool kw_check_address_bounds(uint64_t dev_address, bool is_optional)
{
    /* If the address check is optional,
    Address of zero means that do not verify the address (optional address) */
    if (is_optional && (dev_address == 0))
    {
        return true;
    }

    /* Verify the bounds */
    return ((dev_address >= HOST_MANAGED_DRAM_START) &&
            (dev_address < (atomic_load_local_64(&KW_CB.host_managed_dram_end))));
}

/************************************************************************
*
*   FUNCTION
*
*       kw_cm_abort_wait_timeout_callback
*
*   DESCRIPTION
*
*       Callback for kernel abort wait timeout
*
*   INPUTS
*
*       kw_idx    Kernel worker index
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void kw_cm_abort_wait_timeout_callback(uint8_t kw_idx)
{
    /* Set the flag to indicate timeout */
    atomic_store_local_8(&KW_CB.kernels[kw_idx].cm_abort_wait_timeout_flag, 1);

    /* Trigger IPI to KW */
    syscall(SYSCALL_IPI_TRIGGER_INT, 1ULL << ((KW_BASE_HART_ID + (kw_idx * HARTS_PER_MINION)) % 64),
        MASTER_SHIRE, 0);
}

/************************************************************************
*
*   FUNCTION
*
*       kw_launch_wait_timeout_callback
*
*   DESCRIPTION
*
*       Callback for kernel launch wait timeout
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
static void kw_launch_wait_timeout_callback(uint8_t sqw_idx)
{
    /* Set the kernel abort wait timeout flag */
    atomic_store_local_32(&KW_CB.launch_wait_timeout_flag[sqw_idx], 1U);
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
*       int32_t         status success or error
*
***********************************************************************/
static int32_t kw_wait_for_kernel_launch_flag(uint8_t sqw_idx, uint8_t slot_index)
{
    int32_t status = STATUS_SUCCESS;
    int32_t sw_timer_idx;
    uint32_t timeout_flag;
    cm_kernel_launched_flag_t kernel_launched;

    /* Create timeout to wait for kernel launch completion flag from CM */
    sw_timer_idx = SW_Timer_Create_Timeout(
        &kw_launch_wait_timeout_callback, sqw_idx, KERNEL_LAUNCH_WAIT_TIMEOUT);

    if (sw_timer_idx < 0)
    {
        Log_Write(LOG_LEVEL_ERROR, "SQW[%d]:KW: Unable to register kernel abort wait timeout!\r\n",
            sqw_idx);
        status = KW_ERROR_SW_TIMER_REGISTER_FAIL;
    }
    else
    {
        do
        {
            /* Read the kernel launch flag from MM L2 SCP */
            kernel_launched.flag = atomic_load_global_32(&CM_KERNEL_LAUNCHED_FLAG[slot_index].flag);

            /* Read the timeout flag */
            timeout_flag = atomic_compare_and_exchange_local_32(
                &KW_CB.launch_wait_timeout_flag[sqw_idx], 1, 0);

        } while ((kernel_launched.flag == 0) && (timeout_flag == 0));

        /* Free the registered SW Timeout slot */
        SW_Timer_Cancel_Timeout((uint8_t)sw_timer_idx);

        /* Check for timeout */
        if (timeout_flag == 1)
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
*       int32_t         status success or error
*
***********************************************************************/
static int32_t kw_find_used_kernel_slot(uint16_t launch_tag_id, uint8_t *slot)
{
    int32_t status = KW_ERROR_KERNEL_SLOT_NOT_FOUND;

    /* Acquire the lock */
    acquire_local_spinlock(&KW_CB.resource_lock);

    for (uint8_t i = 0; i < MM_MAX_PARALLEL_KERNELS; i++)
    {
        /* Find the kernel with the given tag ID */
        if (atomic_load_local_16(&KW_CB.kernels[i].launch_tag_id) == launch_tag_id)
        {
            /* Check if the slot is in use */
            if (atomic_load_local_32(&KW_CB.kernels[i].kernel_state) == KERNEL_STATE_IN_USE)
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
*       tag_id              Tag ID of the command
*       slot_index          Index of available KW
*       kernel              Pointer to kernel slot
*
*   OUTPUTS
*
*       int32_t              status success or error
*
***********************************************************************/
static int32_t kw_reserve_kernel_slot(
    uint8_t sqw_idx, uint16_t tag_id, uint8_t *slot_index, kernel_instance_t **kernel)
{
    int32_t status = STATUS_SUCCESS;
    sqw_state_e sqw_state;
    bool slot_reserved = false;

    do
    {
        for (uint8_t i = 0; i < MM_MAX_PARALLEL_KERNELS; i++)
        {
            /* Find unused kernel slot and reserve it */
            if (atomic_compare_and_exchange_local_32(&KW_CB.kernels[i].kernel_state,
                    KERNEL_STATE_UN_USED, KERNEL_STATE_SLOT_RESERVED) == KERNEL_STATE_UN_USED)
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
    if (sqw_state == SQW_STATE_ABORTED)
    {
        status = KW_ABORTED_KERNEL_SLOT_SEARCH;
        Log_Write(
            LOG_LEVEL_ERROR, "TID[%u]:SQW[%d]:KW:ABORTED:kernel slot search\r\n", tag_id, sqw_idx);

        /* Unreserve the slot */
        if (slot_reserved)
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
static void kw_unreserve_kernel_slot(kernel_instance_t *kernel)
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
*       tag_id          Tag ID of the command
*       req_shire_mask  Shire mask of compute minions to be marked in-use
*
*   OUTPUTS
*
*       int32_t          status success or error
*
***********************************************************************/
static int32_t kw_reserve_kernel_shires(uint8_t sqw_idx, uint16_t tag_id, uint64_t req_shire_mask)
{
    int32_t status;
    sqw_state_e sqw_state;

    /* Acquire the lock */
    acquire_local_spinlock(&KW_CB.resource_lock);

    /* Find and wait for the requested shire mask to get free */
    do
    {
        /* Check the required shires are available and ready */
        status = CW_Check_Shires_Available_And_Free(req_shire_mask);

        /* Read the SQW state */
        sqw_state = SQW_Get_State(sqw_idx);
    } while ((status != STATUS_SUCCESS) && (status != CW_SHIRE_UNAVAILABLE) &&
             (sqw_state != SQW_STATE_ABORTED));

    if ((status == STATUS_SUCCESS) && (sqw_state != SQW_STATE_ABORTED))
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
        if (sqw_state == SQW_STATE_ABORTED)
        {
            status = KW_ABORTED_KERNEL_SHIRES_SEARCH;
            Log_Write(LOG_LEVEL_ERROR, "TID[%u]:SQW[%d]:KW:ABORTED:kernel shires search\r\n",
                tag_id, sqw_idx);
        }
        else
        {
            status = KW_ERROR_CW_SHIRES_NOT_READY;
            Log_Write(LOG_LEVEL_ERROR,
                "TID[%u]:SQW[%d]:KW:ERROR:kernel shires unavailable:Requested:0x%lx:Booted:0x%lx\r\n",
                tag_id, sqw_idx, req_shire_mask, CW_Get_Booted_Shires());
            SP_Iface_Report_Error(MM_RECOVERABLE_FW_MM_KW_ERROR, MM_CM_RESERVE_SLOT_ERROR);
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
*       process_kernel_launch_stack_config
*
*   DESCRIPTION
*
*       This function processes the optional stack config present in kernel
*       launch command
*
*   INPUTS
*
*       sqw_idx      Submission queue index
*       cmd          Kernel launch command
*       stack_config Pointer to U-mode stack config
*       launch_args  Pointer to kernel launch args
*
*   OUTPUTS
*
*       int32_t      status success or error
*
***********************************************************************/
static inline int32_t process_kernel_launch_stack_config(uint8_t sqw_idx,
    const struct device_ops_kernel_launch_cmd_t *cmd,
    const struct kernel_user_stack_cfg_t *stack_config,
    mm_to_cm_message_kernel_launch_t *launch_args)
{
    int32_t status = STATUS_SUCCESS;

    /* Get the total enable harts in kernel launch */
    uint32_t harts = get_enabled_umode_harts(cmd->shire_mask, CM_HART_MASK);

    if (harts == 0)
    {
        status = KW_ERROR_KERNEL_UMODE_STACK_INVALID_CONFIG;
        Log_Write(LOG_LEVEL_ERROR,
            "TID[%u]:SQW[%d]:KW:ERROR:No kernel launch harts enabled:Shire mask:0x%lx\r\n",
            cmd->command_info.cmd_hdr.tag_id, sqw_idx, cmd->shire_mask);
    }
    /* Stack size must be atleast 4KB per hart */
    else if (stack_config->stack_size < harts)
    {
        status = KW_ERROR_KERNEL_UMODE_STACK_INVALID_CONFIG;
        Log_Write(LOG_LEVEL_ERROR,
            "TID[%u]:SQW[%d]:KW:ERROR:U-mode stack size not enough:Size(4KB):0x%x:enabled harts:0x%x\r\n",
            cmd->command_info.cmd_hdr.tag_id, sqw_idx, stack_config->stack_size, harts);
    }
    /* Per hart stack size calculation and checks */
    else
    {
        uint32_t hart_stack_size = (stack_config->stack_size * SIZE_4KB) / harts;

        /* Per hart size must be a aligned to cache-line size */
        if (!IS_ALIGNED(hart_stack_size, CACHE_LINE_SIZE))
        {
            status = KW_ERROR_KERNEL_UMODE_STACK_INVALID_CONFIG;
            Log_Write(LOG_LEVEL_ERROR,
                "TID[%u]:SQW[%d]:KW:ERROR:U-mode stack size not aligned to cache-line size:Size(4KB):0x%x:enabled harts:0x%x:per-hart-size(bytes):0x%x\r\n",
                cmd->command_info.cmd_hdr.tag_id, sqw_idx, stack_config->stack_size, harts,
                hart_stack_size);
        }
        else
        {
            /* Save the per hart stack size */
            launch_args->kernel.stack_size = hart_stack_size;
        }
    }

    /* Verify limits of stack start and end */
    if (status == STATUS_SUCCESS)
    {
        /* Compute the start and end address */
        uint64_t start_address =
            HOST_MANAGED_DRAM_START + (stack_config->stack_base_offset * SIZE_4KB);
        uint64_t end_address = start_address + (stack_config->stack_size * SIZE_4KB);

        /* Check limits of addresses */
        if (!kw_check_address_bounds(start_address, false) ||
            !kw_check_address_bounds(end_address, false))
        {
            status = KW_ERROR_KERNEL_UMODE_STACK_INVALID_CONFIG;
            Log_Write(LOG_LEVEL_ERROR,
                "TID[%u]:SQW[%d]:KW:ERROR:U-mode stack params not within limits:Offset(4KB):0x%d:Size(4KB):0x%x:start_add:0x%lx:end_add:0x%lx\r\n",
                cmd->command_info.cmd_hdr.tag_id, sqw_idx, stack_config->stack_base_offset,
                stack_config->stack_size, start_address, end_address);
        }
        else
        {
            /* Save the launch args for kernel worker */
            launch_args->kernel.flags |= KERNEL_LAUNCH_FLAGS_COMPUTE_KERNEL_STACK_CONFIG;
            /* Save the stack base. Stacks go bottom up, hence the end address will be the base */
            launch_args->kernel.stack_base_address = end_address;
        }
    }

    return status;
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
*       sqw_idx     Submission queue index
*       cmd         Kernel launch command
*       launch_args Pointer to kernel launch args
*
*   OUTPUTS
*
*       int32_t      status success or error
*
***********************************************************************/
static inline int32_t process_kernel_launch_cmd_payload(uint8_t sqw_idx,
    struct device_ops_kernel_launch_cmd_t *cmd, mm_to_cm_message_kernel_launch_t *launch_args)
{
    /* Calculate the kernel arguments size */
    uint64_t args_size = (cmd->command_info.cmd_hdr.size - sizeof(*cmd));
    uint8_t *payload = (uint8_t *)cmd->argument_payload;
    Log_Write(LOG_LEVEL_DEBUG, "TID[%u]:SQW[%d]:KW: Kernel launch argument payload size: %ld\r\n",
        cmd->command_info.cmd_hdr.tag_id, sqw_idx, args_size);
    int32_t status = STATUS_SUCCESS;

    if (args_size > DEVICE_OPS_KERNEL_LAUNCH_ARGS_PAYLOAD_MAX)
    {
        status = KW_ERROR_KERNEL_INVALID_ARGS_SIZE;
        Log_Write(LOG_LEVEL_ERROR,
            "TID[%u]:SQW[%d]:KW:ERROR: Violated Kernel Args Size: %ld > %d\r\n",
            cmd->command_info.cmd_hdr.tag_id, sqw_idx, args_size,
            DEVICE_OPS_KERNEL_LAUNCH_ARGS_PAYLOAD_MAX);
    }

    /* Check if Trace config are present in optional command payload. */
    if ((status == STATUS_SUCCESS) &&
        (cmd->command_info.cmd_hdr.flags & CMD_FLAGS_COMPUTE_KERNEL_TRACE_ENABLE))
    {
        Log_Write(LOG_LEVEL_DEBUG, "TID[%u]:SQW[%d]:KW: Trace Optional Payload present!\r\n",
            cmd->command_info.cmd_hdr.tag_id, sqw_idx);
        const struct trace_init_info_t *trace_config =
            (struct trace_init_info_t *)(uintptr_t)payload;

        /* TODO: Add a new deviceApi code for KW_ERROR_KERNEL_TRACE_INVALID_CONFIG */
        /* Perform sanity checks on trace config */
        if (args_size < sizeof(struct trace_init_info_t))
        {
            status = KW_ERROR_KERNEL_TRACE_INVALID_CONFIG;
            Log_Write(LOG_LEVEL_ERROR,
                "TID[%u]:SQW[%d]:KW:ERROR:Invalid trace config Size: %ld\r\n",
                cmd->command_info.cmd_hdr.tag_id, sqw_idx, args_size);
        }
        else if (!IS_ALIGNED(trace_config->buffer, CACHE_LINE_SIZE) ||
                 !IS_ALIGNED(trace_config->buffer_size, CACHE_LINE_SIZE))
        {
            status = KW_ERROR_KERNEL_TRACE_INVALID_CONFIG;
            Log_Write(LOG_LEVEL_ERROR,
                "TID[%u]:SQW[%d]:KW:ERROR:Unaligned Trace Buffer:0x%ld:Size:0x%xr\n",
                cmd->command_info.cmd_hdr.tag_id, sqw_idx, trace_config->buffer,
                trace_config->buffer_size);
        }

        if (status == STATUS_SUCCESS)
        {
            /* Get the total enable harts in U-mode trace */
            uint32_t harts =
                get_enabled_umode_harts(trace_config->shire_mask, trace_config->thread_mask);

            /* Verify the enabled harts and total buffer size available for them */
            if (harts == 0)
            {
                status = KW_ERROR_KERNEL_TRACE_INVALID_CONFIG;
                Log_Write(LOG_LEVEL_ERROR,
                    "TID[%u]:SQW[%d]:KW:ERROR:No U-mode trace harts enabled:Shire mask:0x%lx:Thread mask:0x%lx\r\n",
                    cmd->command_info.cmd_hdr.tag_id, sqw_idx, trace_config->shire_mask,
                    trace_config->thread_mask);
            }
            else if ((trace_config->buffer_size / harts) < CACHE_LINE_SIZE)
            {
                status = KW_ERROR_KERNEL_TRACE_INVALID_CONFIG;
                Log_Write(LOG_LEVEL_ERROR,
                    "TID[%u]:SQW[%d]:KW:ERROR:U-mode trace buffer size must be atleast cache-line size:buffer size:0x%x:harts:0x%x\r\n",
                    cmd->command_info.cmd_hdr.tag_id, sqw_idx, trace_config->buffer_size, harts);
            }
            else
            {
                /* Since the U-mode trace config is per kernel slot, the contents from
                command to the assigned address in S-mode will be done after reserving the slot */

                /* Set the flag in kernel launch args */
                launch_args->kernel.flags |= KERNEL_LAUNCH_FLAGS_COMPUTE_KERNEL_TRACE_ENABLE;

                /* Just increment the pointer and size */
                args_size -= sizeof(struct trace_init_info_t);
                payload += sizeof(struct trace_init_info_t);
            }
        }
    }

    /* Check if Umode stack config is present in optional command payload. */
    if ((status == STATUS_SUCCESS) &&
        (cmd->command_info.cmd_hdr.flags & CMD_FLAGS_KERNEL_LAUNCH_USER_STACK_CFG))
    {
        Log_Write(LOG_LEVEL_DEBUG, "TID[%u]:SQW[%d]:KW: kernel U-mode stack config!\r\n",
            cmd->command_info.cmd_hdr.tag_id, sqw_idx);

        const struct kernel_user_stack_cfg_t *stack_config =
            (struct kernel_user_stack_cfg_t *)(uintptr_t)payload;

        /* Perform sanity checks on stack config */
        if (args_size < sizeof(struct kernel_user_stack_cfg_t))
        {
            status = KW_ERROR_KERNEL_UMODE_STACK_INVALID_CONFIG;
            Log_Write(LOG_LEVEL_ERROR,
                "TID[%u]:SQW[%d]:KW:ERROR:Invalid U-mode stack config size: %ld\r\n",
                cmd->command_info.cmd_hdr.tag_id, sqw_idx, args_size);
        }
        else
        {
            /* Process the kernel launch U-mode stack config */
            status = process_kernel_launch_stack_config(sqw_idx, cmd, stack_config, launch_args);

            /* Increment the pointer and size */
            if (status == STATUS_SUCCESS)
            {
                args_size -= sizeof(struct kernel_user_stack_cfg_t);
                payload += sizeof(struct kernel_user_stack_cfg_t);
            }
        }
    }

    /* Check if Kernel arguments are present in optional command payload. */
    if ((status == STATUS_SUCCESS) &&
        (cmd->command_info.cmd_hdr.flags & CMD_FLAGS_KERNEL_LAUNCH_ARGS_EMBEDDED) &&
        (cmd->pointer_to_args != 0))
    {
        Log_Write(LOG_LEVEL_DEBUG, "TID[%u]:SQW[%d]:KW: Kernel Args Optional Payload present!\r\n",
            cmd->command_info.cmd_hdr.tag_id, sqw_idx);
        /* Copy the kernel arguments from command payload to provided address
           NOTE: Kernel argument position depends upon other optional fields in payload,
                 if there is no other optional payload then all data in payload is kernel args. */
        ETSOC_MEM_COPY_AND_EVICT(
            (void *)(uintptr_t)cmd->pointer_to_args, (void *)payload, args_size, to_L3)
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
*       int32_t      status success or error
*
***********************************************************************/
int32_t KW_Dispatch_Kernel_Launch_Cmd(
    struct device_ops_kernel_launch_cmd_t *cmd, uint8_t sqw_idx, uint8_t *kw_idx)
{
    kernel_instance_t *kernel = 0;
    mm_to_cm_message_kernel_launch_t launch_args = { 0 };
    int32_t status = KW_ERROR_KERNEL_INVALID_ADDRESS;
    uint8_t slot_index;

    /* Verify the shire mask */
    if (cmd->shire_mask == 0)
    {
        Log_Write(LOG_LEVEL_ERROR, "TID[%u]:SQW[%d]:KW:ERROR:Invalid Shire Mask:0x%lx\r\n",
            cmd->command_info.cmd_hdr.tag_id, sqw_idx, cmd->shire_mask);
        return KW_ERROR_KERNEL_INVALID_SHIRE_MASK;
    }

    /* Verify address bounds
       kernel start address (not optional)
       different addresses provided in the command could be optional address. */
    if (kw_check_address_bounds(cmd->code_start_address, false) &&
        kw_check_address_bounds(cmd->pointer_to_args, true) &&
        kw_check_address_bounds(cmd->exception_buffer, true))
    {
        /* Process the kernel launch cmd optional payload */
        status = process_kernel_launch_cmd_payload(sqw_idx, cmd, &launch_args);
    }

    if (status == STATUS_SUCCESS)
    {
        /* Populate the initial kernel launch CM msg params */
        launch_args.header.id = MM_TO_CM_MESSAGE_ID_KERNEL_LAUNCH;
        launch_args.header.tag_id = cmd->command_info.cmd_hdr.tag_id;
        launch_args.header.flags = CM_IFACE_FLAG_ASYNC_CMD;
        launch_args.kernel.kw_base_id = (uint8_t)KW_MS_BASE_HART;
        launch_args.kernel.code_start_address = cmd->code_start_address;
        launch_args.kernel.pointer_to_args = cmd->pointer_to_args;
        launch_args.kernel.shire_mask = cmd->shire_mask;
        launch_args.kernel.exception_buffer = cmd->exception_buffer;

        /* If the flag bit flush L3 is set */
        if (cmd->command_info.cmd_hdr.flags & CMD_FLAGS_KERNEL_LAUNCH_FLUSH_L3)
        {
            launch_args.kernel.flags |= KERNEL_LAUNCH_FLAGS_EVICT_L3_BEFORE_LAUNCH;
        }

        /* First we allocate resources needed for the kernel launch */
        /* Reserve a slot for the kernel */
        status =
            kw_reserve_kernel_slot(sqw_idx, cmd->command_info.cmd_hdr.tag_id, &slot_index, &kernel);

        if (status == STATUS_SUCCESS)
        {
            /* Reserve compute shires needed for the requested
            kernel launch */
            status = kw_reserve_kernel_shires(
                sqw_idx, cmd->command_info.cmd_hdr.tag_id, cmd->shire_mask);

            if (status == STATUS_SUCCESS)
            {
                atomic_store_local_64(&kernel->kernel_shire_mask, cmd->shire_mask);
            }
            else
            {
                /* Make reserved kernel slot available again */
                kw_unreserve_kernel_slot(kernel);
            }
        }

        if (status == STATUS_SUCCESS)
        {
            /* Populate the assigned slot ID in the CM kernel launch msg */
            launch_args.kernel.slot_index = slot_index;

            /* Copy the CM U-mode config to S-mode region - config verification was done above */
            KW_COPY_CM_UMODE_TRACE_CFG_OPTIONALLY(slot_index, cmd)

            /* Setup kernel environment shire mask */
            KW_INIT_KERNEL_ENV_SHIRE_MASK(slot_index, cmd->shire_mask)

            /* Populate the tag_id and sqw_idx for KW */
            atomic_store_local_16(&kernel->launch_tag_id, cmd->command_info.cmd_hdr.tag_id);
            atomic_store_local_8(&kernel->sqw_idx, sqw_idx);

            /* Reset the L2 SCP kernel launched flag for the acquired kernel worker slot */
            atomic_store_global_32(&CM_KERNEL_LAUNCHED_FLAG[slot_index].flag, 0);

            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_EXECUTING)

            /* Blocking call that blocks till all shires ack command */
            status = CM_Iface_Multicast_Send(
                launch_args.kernel.shire_mask, (cm_iface_message_t *)&launch_args);

            if (status == STATUS_SUCCESS)
            {
                /* Mark the kernel slot in use since the kernel is launched */
                atomic_store_local_32(&kernel->kernel_state, KERNEL_STATE_IN_USE);
                *kw_idx = slot_index;
                atomic_store_local_64(&kernel->umode_exception_buffer_ptr, cmd->exception_buffer);

                /* Save the U-mode trace ptr in KW CB */
                KW_SAVE_UMODE_TRACE_PTR(kernel, cmd)
            }
            else
            {
                Log_Write(LOG_LEVEL_ERROR,
                    "TID[%u]:SQW[%d]:KW:ERROR:MM2CMLaunch:CommandMulticast:Failed:Status:%d\r\n",
                    cmd->command_info.cmd_hdr.tag_id, sqw_idx, status);

                /* Broadcast message failed. Reclaim resources */
                kw_unreserve_kernel_shires(cmd->shire_mask);
                kw_unreserve_kernel_slot(kernel);

                SP_Iface_Report_Error(
                    MM_RECOVERABLE_FW_MM_KW_ERROR, MM_CM_MULTICAST_KERNEL_LAUNCH_ERROR);
                status = KW_ERROR_CM_IFACE_MULTICAST_FAILED;
            }
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
*       int32_t      status success or error
*
***********************************************************************/
int32_t KW_Dispatch_Kernel_Abort_Cmd(
    const struct device_ops_kernel_abort_cmd_t *cmd, uint8_t sqw_idx)
{
    int32_t status;
    uint8_t slot_index;
    cm_iface_message_t message = { 0 };
    struct device_ops_kernel_abort_rsp_t abort_rsp;

    /* Find the kernel associated with the given tag_id */
    status = kw_find_used_kernel_slot(cmd->kernel_launch_tag_id, &slot_index);

    if (status == STATUS_SUCCESS)
    {
        /* Wait until all the harts on CM side have launched kernel */
        status = kw_wait_for_kernel_launch_flag(sqw_idx, slot_index);

        if (status == STATUS_SUCCESS)
        {
            /* Update the kernel state to aborted */
            atomic_store_local_32(
                &KW_CB.kernels[slot_index].kernel_state, KERNEL_STATE_ABORTED_BY_HOST);

            /* Set the kernel abort message */
            message.header.id = MM_TO_CM_MESSAGE_ID_KERNEL_ABORT;
            message.header.tag_id = cmd->command_info.cmd_hdr.tag_id;
            message.header.flags = CM_IFACE_FLAG_ASYNC_CMD;

            /* Command status trace log */
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_CMD, sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_EXECUTING)

            /* Blocking call that blocks till all shires ack */
            status = CM_Iface_Multicast_Send(
                atomic_load_local_64(&KW_CB.kernels[slot_index].kernel_shire_mask), &message);

            /* Construct and transmit kernel abort response to host */
            abort_rsp.response_info.rsp_hdr.tag_id = cmd->command_info.cmd_hdr.tag_id;
            abort_rsp.response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP;
            abort_rsp.response_info.rsp_hdr.size = sizeof(abort_rsp) - sizeof(struct cmn_header_t);

            /* Check the multicast send for errors */
            if (status == STATUS_SUCCESS)
            {
                abort_rsp.status = DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS;
            }
            else
            {
                abort_rsp.status = DEV_OPS_API_KERNEL_ABORT_RESPONSE_ERROR;
                SP_Iface_Report_Error(
                    MM_RECOVERABLE_FW_MM_KW_ERROR, MM_CM_MULTICAST_KERNEL_ABORT_ERROR);
                Log_Write(LOG_LEVEL_ERROR,
                    "SQW[%d]:KW:ERROR:MM2CMAbort:CommandMulticast:Failed\r\n", sqw_idx);
            }

            /* Send kernel abort response to host */
            status = Host_Iface_CQ_Push_Cmd(0, &abort_rsp, sizeof(abort_rsp));

            if (status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG,
                    "TID[%u]:SQW[%d]:KW:Pushed:KERNEL_ABORT_CMD_RSP->Host_CQ\r\n",
                    abort_rsp.response_info.rsp_hdr.tag_id, sqw_idx);
            }
            else
            {
                SP_Iface_Report_Error(MM_RECOVERABLE_FW_MM_KW_ERROR, MM_CQ_PUSH_ERROR);
                Log_Write(
                    LOG_LEVEL_ERROR, "SQW[%d]:KW:Push:KERNEL_ABORT_CMD_RSP:Failed\r\n", sqw_idx);
            }

            /* Decrement commands count being processed by given SQW */
            SQW_Decrement_Command_Count(sqw_idx);
        }
        else
        {
            /* Report error to SP */
            SP_Iface_Report_Error(MM_RECOVERABLE_FW_MM_KW_ERROR, MM_CM_KERNEL_ABORT_TIMEOUT_ERROR);
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
    Log_Write(LOG_LEVEL_WARNING, "SQW[%d]:KW:Abort all kernels\r\n", sqw_idx);

    /* Traverse all kernel slots and abort them */
    for (uint8_t kw_idx = 0; kw_idx < MM_MAX_PARALLEL_KERNELS; kw_idx++)
    {
        /* Spin-wait if kernel slot state is reserved.
        Reserved slot needs to transition to unused or in use before we can abort it */
        do
        {
            asm volatile("fence\n" ::: "memory");
        } while (atomic_load_local_32(&KW_CB.kernels[kw_idx].kernel_state) ==
                 KERNEL_STATE_SLOT_RESERVED);

        /* Check if this kernel slot is used by the given sqw_idx
        and kernel slot is in use, then abort it */
        if ((atomic_load_local_8(&KW_CB.kernels[kw_idx].sqw_idx) == sqw_idx) &&
            (atomic_compare_and_exchange_local_32(&KW_CB.kernels[kw_idx].kernel_state,
                 KERNEL_STATE_IN_USE, KERNEL_STATE_ABORTING) == KERNEL_STATE_IN_USE))
        {
            Log_Write(LOG_LEVEL_DEBUG, "SQW[%d]:KW:Aborting KW=%d\r\n", sqw_idx, kw_idx);

            /* Trigger IPI to KW */
            syscall(SYSCALL_IPI_TRIGGER_INT,
                1ULL << ((KW_BASE_HART_ID + (kw_idx * HARTS_PER_MINION)) % 64), MASTER_SHIRE, 0);

            /* Spin-wait if the KW state is aborting */
            do
            {
                asm volatile("fence\n" ::: "memory");
            } while (
                atomic_load_local_32(&KW_CB.kernels[kw_idx].kernel_state) == KERNEL_STATE_ABORTING);

            Log_Write(LOG_LEVEL_DEBUG, "SQW[%d]:KW:Aborted KW=%d\r\n", sqw_idx, kw_idx);
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
    for (uint32_t i = 0; i < MM_MAX_PARALLEL_KERNELS; i++)
    {
        /* Initialize FCC flags used by the kernel worker */
        atomic_store_local_8(&KW_CB.host2kw[i].fcc_id, FCC_0);
        global_fcc_init(&KW_CB.host2kw[i].fcc_flag);

        /* Initialize the tag IDs, sqw_idx and kernel state */
        atomic_store_local_64((uint64_t *)&KW_CB.kernels[i], 0U);

        /* Initialize shire_mask, wait and start cycle count */
        atomic_store_local_64(&KW_CB.kernels[i].kernel_shire_mask, 0U);
        atomic_store_local_64(&KW_CB.kernels[i].kw_cycles.exec_start_cycles, 0U);
        atomic_store_local_64(&KW_CB.kernels[i].kw_cycles.cmd_start_cycles, 0U);
        atomic_store_local_64(&KW_CB.kernels[i].kw_cycles.wait_cycles, 0U);
        atomic_store_local_64(&KW_CB.kernels[i].kw_cycles.prev_cycles, 0U);

        kernel_environment_t *kernel_env =
            (kernel_environment_t *)(CM_KERNEL_ENVS_BASEADDR + i * KERNEL_ENV_SIZE);

        /* Fill the kernel slot environment - these properties are filled once at boot time */
        kernel_env->version.major = ABI_VERSION_MAJOR;
        kernel_env->version.minor = ABI_VERSION_MINOR;
        kernel_env->version.patch = ABI_VERSION_PATCH;
        kernel_env->frequency = MM_Config_Get_Minion_Boot_Freq();

        /* Evict the data to L2 SCP */
        ETSOC_MEM_EVICT((void *)(uintptr_t)kernel_env, sizeof(kernel_environment_t), to_L2)
    }

    /* Initialize DDR size */
    atomic_store_local_64(&KW_CB.host_managed_dram_end, MM_Config_Get_DRAM_End_Address());

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
void KW_Notify(uint8_t kw_idx, const execution_cycles_t *cycle)
{
    uint32_t minion = KW_WORKER_0 + kw_idx;

    Log_Write(LOG_LEVEL_DEBUG, "KW[%d]:Notifying:KW:minion=%d:thread=%d\r\n", kw_idx, minion,
        KW_THREAD_ID);

    atomic_store_local_64(
        (void *)&KW_CB.kernels[kw_idx].kw_cycles.cmd_start_cycles, cycle->cmd_start_cycles);
    atomic_store_local_64(
        (void *)&KW_CB.kernels[kw_idx].kw_cycles.exec_start_cycles, cycle->exec_start_cycles);
    atomic_store_local_64((void *)&KW_CB.kernels[kw_idx].kw_cycles.wait_cycles, cycle->wait_cycles);

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
*
*   OUTPUTS
*
*       int32_t             status success or error
*
***********************************************************************/
static inline int32_t kw_cm_to_mm_kernel_force_abort(uint64_t kernel_shire_mask)
{
    cm_iface_message_t abort_msg = { .header.id = MM_TO_CM_MESSAGE_ID_KERNEL_ABORT,
        .header.tag_id = 0,
        .header.flags = CM_IFACE_FLAG_ASYNC_CMD };

    int32_t status;

    Log_Write(LOG_LEVEL_DEBUG, "KW:MM->CM:Sending abort multicast msg.\r\n");

    /* Blocking call (with timeout) that blocks till all shires ack */
    status = CM_Iface_Multicast_Send(kernel_shire_mask, &abort_msg);

    /* Verify that there is no abort hang situation recovery failure */
    if (status != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "KW:MM->CM:Abort Cmd hanged.status:%d\r\n", status);

        /* Report the error to SP */
        SP_Iface_Report_Error(MM_RECOVERABLE_FW_MM_KW_ERROR, MM_CM_MULTICAST_KERNEL_ABORT_ERROR);

        status = KW_ERROR_CM_IFACE_MULTICAST_FAILED;
    }
    else
    {
        Log_Write(LOG_LEVEL_DEBUG, "KW:MM->CM:Abort multicast complete.\r\n");
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
*       Helper function to proccess CM-to-MM messages.
*
*   INPUTS
*
*       kw_id               Index of kernel worker.
*       tag_id              Tag ID of the kernel command.
*       kernel_shire_mask   Shire mask of compute workers.
*       status_internal     Status control block to return status.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static inline void kw_cm_to_mm_process_messages(uint32_t kw_idx, uint16_t tag_id,
    uint64_t kernel_shire_mask, struct kw_internal_status *status_internal)
{
    cm_iface_message_t message;
    int32_t status;

    Log_Write(LOG_LEVEL_DEBUG, "KW[%d]:Processing msgs from CMs\r\n", kw_idx);

    /* Process all messages until the buffer is empty */
    while (1)
    {
        /* Receive the CM->MM message */
        status = CM_Iface_Unicast_Receive(CM_MM_KW_HART_UNICAST_BUFF_BASE_IDX + kw_idx, &message);

        if (status != STATUS_SUCCESS)
        {
            /* No more pending messages left */
            if ((status != CIRCBUFF_ERROR_BAD_LENGTH) && (status != CIRCBUFF_ERROR_EMPTY))
            {
                status_internal->status = KW_ERROR_CM_IFACE_UNICAST_FAILED;

                SP_Iface_Report_Error(MM_RECOVERABLE_FW_MM_KW_ERROR, MM_KW_UNICAST_RECEIVE_ERROR);

                Log_Write(LOG_LEVEL_ERROR,
                    "KW[%d]:ERROR:CM_To_MM Receive failed. Status code: %d\r\n", kw_idx, status);
            }

            Log_Write(LOG_LEVEL_DEBUG, "KW[%d]:CM_To_MM: No pending msg\r\n", kw_idx);

            break;
        }

        switch (message.header.id)
        {
            case CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE:
                /* Set the flag to indicate that kernel launch has completed */
                status_internal->kernel_done = true;

                const cm_to_mm_message_kernel_launch_completed_t *completed =
                    (cm_to_mm_message_kernel_launch_completed_t *)&message;

                Log_Write(LOG_LEVEL_DEBUG,
                    "TID[%u]:KW[%d]:from CW:CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE from S%d:Status:%d\r\n",
                    tag_id, kw_idx, completed->shire_id, completed->status);

                /* Check the completion status for any error. */
                if (completed->status == KERNEL_COMPLETE_STATUS_ERROR)
                {
                    status_internal->cw_error = true;
                    status_internal->cm_error_shire_mask = completed->exception_mask |
                                                           completed->system_abort_mask;

                    Log_Write(LOG_LEVEL_ERROR,
                        "TID[%u]:KW[%d]:CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE:Execution error detected:S%d:Exception Mask:0x%lx:Abort Mask:0x%lx\r\n",
                        tag_id, kw_idx, completed->shire_id, completed->exception_mask,
                        completed->system_abort_mask);
                }
                break;

            case CM_TO_MM_MESSAGE_ID_KERNEL_EXCEPTION:
            {
                const cm_to_mm_message_exception_t *exception =
                    (cm_to_mm_message_exception_t *)&message;

                Log_Write(LOG_LEVEL_ERROR,
                    "TID[%u]:KW[%d]:from CW:CM_TO_MM_MESSAGE_ID_KERNEL_EXCEPTION from S%d H%lu @ SEPC: 0x%lX SCAUSE: 0x%lX STVAL: 0x%lX SSTATUS: 0x%lX \r\n",
                    tag_id, kw_idx, exception->shire_id, exception->hart_id, exception->mepc,
                    exception->mcause, exception->mtval, exception->mstatus);

                /* Save shire mask which took exception. */
                if (!status_internal->cw_exception)
                {
                    status_internal->cw_exception = true;
                }
                break;
            }
            case CM_TO_MM_MESSAGE_ID_KERNEL_LAUNCH_ERROR:
            {
                const cm_to_mm_message_kernel_launch_error_t *error_mesg =
                    (cm_to_mm_message_kernel_launch_error_t *)&message;

                Log_Write(LOG_LEVEL_ERROR,
                    "TID[%u]:KW[%d]:from CW:CM_TO_MM_MESSAGE_ID_KERNEL_LAUNCH_ERROR from H%" PRId64
                    "\r\n",
                    tag_id, kw_idx, error_mesg->hart_id);

                /* Fatal error received. Try to recover kernel shires by sending abort message */
                status_internal->status = kw_cm_to_mm_kernel_force_abort(kernel_shire_mask);

                /* Set error and done flag */
                status_internal->cw_error = true;
                status_internal->kernel_done = true;
                break;
            }
            default:
                Log_Write(LOG_LEVEL_ERROR, "TID[%u]:KW[%d]:from CW: Unexpected msg. ID: %d\r\n",
                    kw_idx, tag_id, message.header.id);

                /* Report SP of unknown msg Error */
                SP_Iface_Report_Error(
                    MM_RECOVERABLE_FW_CM_RUNTIME_ERROR, MM_KW_UNKNOWN_MESSAGE_ERROR);
                break;
        }
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
static inline uint32_t kw_get_kernel_launch_completion_status(
    uint32_t kernel_state, const struct kw_internal_status *status_internal)
{
    uint32_t status;

    /* These checks below are in order of priority */
    if ((kernel_state == KERNEL_STATE_ABORTED_BY_HOST) || (kernel_state == KERNEL_STATE_ABORTING))
    {
        /* Check for any errors */
        if (status_internal->status == STATUS_SUCCESS)
        {
            /* Update the kernel launch response to indicate that it was aborted by host */
            status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED;
        }
        else if (status_internal->status == KW_ERROR_CW_MINIONS_BOOT_FAILED)
        {
            status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_CW_MINIONS_BOOT_FAILED;
        }
        else if (status_internal->status == KW_ERROR_SP_IFACE_RESET_FAILED)
        {
            status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_SP_IFACE_RESET_FAILED;
        }
        else if (status_internal->status == KW_ERROR_CM_IFACE_MULTICAST_FAILED)
        {
            status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_CM_IFACE_MULTICAST_FAILED;
        }
        else if (status_internal->status == KW_ERROR_CM_ABORT_TIMEOUT)
        {
            /* TODO: Add specific error code for CM abort completion timeout */
            status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_CM_IFACE_MULTICAST_FAILED;
        }
        else
        {
            /* It should never come here. */
            status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_UNEXPECTED_ERROR;
        }
    }
    else if (status_internal->cw_exception)
    {
        /* Exception was detected in kernel run, update response */
        status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_EXCEPTION;
    }
    else if (status_internal->cw_error)
    {
        /* Error was detected in kernel run, update response */
        status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_ERROR;
    }
    else if (status_internal->status == KW_ERROR_CM_IFACE_UNICAST_FAILED)
    {
        status = DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_CM_IFACE_UNICAST_FAILED;
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
__attribute__((noreturn)) void KW_Launch(uint32_t kw_idx)
{
    bool wait_for_ipi = true;
    bool kw_abort_serviced;
    uint8_t local_sqw_idx;
    uint16_t tag_id;
    int32_t status;
    int32_t kw_abort_timer;
    uint32_t kernel_state;
    uint64_t kernel_shire_mask;
    struct kw_internal_status status_internal;
    /* Allocate memory for response message, it includes optional payload. */
    uint8_t rsp_data[sizeof(struct device_ops_kernel_launch_rsp_t) +
                     sizeof(struct kernel_rsp_error_ptr_t)] __attribute__((aligned(8))) = { 0 };
    struct device_ops_kernel_launch_rsp_t *launch_rsp =
        (struct device_ops_kernel_launch_rsp_t *)(uintptr_t)rsp_data;

    /* Get the kernel instance */
    kernel_instance_t *const kernel = &KW_CB.kernels[kw_idx];

    Log_Write(LOG_LEVEL_INFO, "KW[%d]\r\n", kw_idx);

    while (1)
    {
        /* Wait on FCC notification from SQW Host Command Handler */
        global_fcc_wait(
            atomic_load_local_8(&KW_CB.host2kw[kw_idx].fcc_id), &KW_CB.host2kw[kw_idx].fcc_flag);

        Log_Write(LOG_LEVEL_DEBUG, "KW:Received:FCCEvent\r\n");

        /* Reset state */
        status_internal.kernel_done = false;
        status_internal.cw_exception = false;
        status_internal.cw_error = false;
        kw_abort_serviced = false;
        status_internal.status = STATUS_SUCCESS;
        status_internal.cm_error_shire_mask = 0;
        wait_for_ipi = true;
        kw_abort_timer = -1;
        atomic_store_local_8(&kernel->cm_abort_wait_timeout_flag, 0);

        /* Read the shire mask and tag ID for the current kernel */
        kernel_shire_mask = atomic_load_local_64(&kernel->kernel_shire_mask);
        tag_id = atomic_load_local_16(&kernel->launch_tag_id);

        /* Process kernel command responses from CM, for all shires
        associated with the kernel launch */
        while (!status_internal.kernel_done && (status_internal.status == STATUS_SUCCESS))
        {
            /* Wait and clear IPI */
            KW_WAIT_AND_CLEAR_SW_INTERRUPT(wait_for_ipi)

            /* Get the kernel state */
            kernel_state = atomic_load_local_32(&kernel->kernel_state);

            /* Check the kernel_state is set to abort after timeout */
            if ((!kw_abort_serviced) && (kernel_state == KERNEL_STATE_ABORTING))
            {
                kw_abort_serviced = true;
                Log_Write(LOG_LEVEL_ERROR, "TID[%u]:KW[%d]:Aborting kernel...\r\n", tag_id, kw_idx);

                /* Make sure that the kernel is launched on the CMs */
                kw_wait_for_kernel_launch_flag(
                    atomic_load_local_8(&kernel->sqw_idx), (uint8_t)kw_idx);

                /* Multicast abort to shires associated with current kernel slot
                This abort should forcefully abort all the shires involved in kernel launch */
                status_internal.status = kw_cm_to_mm_kernel_force_abort(kernel_shire_mask);

                /* Check and register a timer for completion message from CMs after abort */
                KW_REGISTER_CM_ABORT_TIMER(kw_abort_timer, kw_idx, status_internal.status)

                /* Since we did a multicast to CMs in above call, being pessimistic here
                and disabling the wait for IPI to make sure we don't miss any pending message. */
                wait_for_ipi = false;
            }
            else if ((kernel_state == KERNEL_STATE_ABORTING) &&
                     (atomic_load_local_8(&kernel->cm_abort_wait_timeout_flag) == 1))
            {
                /* Set the status to indicate that timeout occured while waiting for abort completion message from CMs. */
                status_internal.status = KW_ERROR_CM_ABORT_TIMEOUT;

                Log_Write(LOG_LEVEL_ERROR,
                    "TID[%u]:KW[%d]:Timeout occured waiting for kernel complete message after CM abort\r\n",
                    tag_id, kw_idx);
            }
            else
            {
                /* Handle messages from Compute Worker */
                kw_cm_to_mm_process_messages(kw_idx, tag_id, kernel_shire_mask, &status_internal);

                /* Enable wait for IPI */
                wait_for_ipi = true;
            }
        }

        /* Check if CM abort timer was registered */
        if (kw_abort_timer >= 0)
        {
            /* Free the registered SW Timeout slot */
            SW_Timer_Cancel_Timeout((uint8_t)kw_abort_timer);
        }

        /* Kernel run complete with host abort, exception or success.
        reclaim resources and Prepare response */

        uint16_t rsp_size = (uint16_t)(sizeof(struct device_ops_kernel_launch_rsp_t));

        /* Read the kernel state to detect abort by host */
        kernel_state = atomic_load_local_32(&kernel->kernel_state);

        /* Construct and transmit kernel launch response to host */
        launch_rsp->response_info.rsp_hdr.tag_id = tag_id;
        launch_rsp->response_info.rsp_hdr.msg_id = DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP;
        launch_rsp->device_cmd_start_ts = atomic_load_local_64(&kernel->kw_cycles.cmd_start_cycles);
        launch_rsp->device_cmd_wait_dur = atomic_load_local_64(&kernel->kw_cycles.wait_cycles);
        launch_rsp->device_cmd_execute_dur =
            PMC_GET_LATENCY(atomic_load_local_64(&kernel->kw_cycles.exec_start_cycles));

        local_sqw_idx = atomic_load_local_8(&kernel->sqw_idx);

        /* Get completion status of kernel launch. */
        launch_rsp->status = kw_get_kernel_launch_completion_status(kernel_state, &status_internal);

        if (launch_rsp->status != DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED)
        {
            /* Kernel was not completed successfully. So populate response with exception and trace buffer pointers. */
            struct kernel_rsp_error_ptr_t error_ptrs;
            error_ptrs.umode_exception_buffer_ptr =
                atomic_load_local_64(&kernel->umode_exception_buffer_ptr);
            error_ptrs.umode_trace_buffer_ptr =
                atomic_load_local_64(&kernel->umode_trace_buffer_ptr);
            error_ptrs.cm_shire_mask = status_internal.cm_error_shire_mask;

            /* Copy the error pointers (which is optional payload) at the end of response.
               NOTE: Memory for optional payload is already allocated, so it is safe to use memory beyond normal response size. */
            memcpy(&launch_rsp->kernel_rsp_error_ptr[0], &error_ptrs, sizeof(error_ptrs));
            rsp_size = (uint16_t)(rsp_size + sizeof(error_ptrs));
        }

        /* Give back the reserved compute shires. */
        kw_unreserve_kernel_shires(kernel_shire_mask);

        /* Make reserved kernel slot available again */
        kw_unreserve_kernel_slot(kernel);

#if TEST_FRAMEWORK
        /* For SP2MM command response, we need to provide the total size = header + payload */
        launch_rsp->response_info.rsp_hdr.size = rsp_size;
        /* Send kernel launch response to SP */
        status = SP_Iface_Push_Rsp_To_SP2MM_CQ(launch_rsp, rsp_size);
#else
        launch_rsp->response_info.rsp_hdr.size = (uint16_t)(rsp_size - sizeof(struct cmn_header_t));
        /* Send kernel launch response to host */
        status = Host_Iface_CQ_Push_Cmd(0, launch_rsp, rsp_size);
#endif

        /* Accumlate kernel execution cycles. */
        atomic_add_local_64(&kernel->kernel_exec_cycles, launch_rsp->device_cmd_execute_dur);

        /* Update kernel running time for stats Trace. Reporting unit is kernels/second */
        STATW_Add_New_Sample_Atomically(STATW_RESOURCE_CM,
            (STATW_Get_Minion_Freq() * 1000000UL / launch_rsp->device_cmd_execute_dur));

        if (status == STATUS_SUCCESS)
        {
            /* Log to command status to trace */
            if (launch_rsp->status == DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED)
            {
                TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD, local_sqw_idx,
                    launch_rsp->response_info.rsp_hdr.tag_id, CMD_STATUS_SUCCEEDED);
            }
            else if (launch_rsp->status == DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED)
            {
                TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD, local_sqw_idx,
                    launch_rsp->response_info.rsp_hdr.tag_id, CMD_STATUS_ABORTED);
            }
            else
            {
                TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD, local_sqw_idx,
                    launch_rsp->response_info.rsp_hdr.tag_id, CMD_STATUS_FAILED);
            }

            Log_Write(LOG_LEVEL_DEBUG, "TID[%u]:KW[%d]:CQ_Push:KERNEL_LAUNCH_CMD_RSP\r\n",
                launch_rsp->response_info.rsp_hdr.tag_id, kw_idx);
        }
        else
        {
            TRACE_LOG_CMD_STATUS(DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD, local_sqw_idx,
                launch_rsp->response_info.rsp_hdr.tag_id, CMD_STATUS_FAILED);

            Log_Write(LOG_LEVEL_ERROR, "KW[%d]:CQ_Push:Failed\r\n", kw_idx);
            SP_Iface_Report_Error(MM_RECOVERABLE_FW_MM_KW_ERROR, MM_CQ_PUSH_ERROR);
        }

#if !TEST_FRAMEWORK
        /* Decrement commands count being processed by given SQW */
        SQW_Decrement_Command_Count(local_sqw_idx);

        /* Check for device API error */
        if (launch_rsp->status != DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED)
        {
            /* Report device API error to SP */
            SP_Iface_Report_Error(
                MM_RECOVERABLE_OPS_API_KERNEL_LAUNCH, (int16_t)launch_rsp->status);
        }
#else
        (void)local_sqw_idx;
#endif
    } /* loop forever */

    /* will not return */
}

/************************************************************************
*
*   FUNCTION
*
*       KW_Get_Average_Exec_Cycles
*
*   DESCRIPTION
*
*       This function gets Compute Minion utlization in terms of cycles.
*       It calculates the average of all kernel slots.
*
*   INPUTS
*
*       interval_start      start cycles of sampling interval
*       interval_end        end cycles of sampling interval
*
*   OUTPUTS
*
*       uint64_t    Average consumed cycles
*
***********************************************************************/
uint64_t KW_Get_Average_Exec_Cycles(uint64_t interval_start, uint64_t interval_end)
{
    uint64_t accum_cycles = 0;
    uint8_t active_kw = 0;
    uint64_t exec_start_cycles = 0;
    uint64_t exec_end_cycles = 0;
    kernel_instance_t *kernel;
    uint64_t total_shire_count = get_set_bit_count(CW_Get_Booted_Shires());
    uint64_t kw_tans_cycles = 0;
    uint64_t interval_cycles = (interval_end - interval_start);

    if (total_shire_count == 0)
    {
        Log_Write(LOG_LEVEL_INFO, "KW_Get_Average_Exec_Cycles:No CM shires available.\r\n");
        return 0;
    }

    for (int kw = 0; kw < KW_NUM; kw++)
    {
        kernel = &KW_CB.kernels[kw];

        /* get kernel and execution start cycles to calculate execution end cycles.
        this will help identifying kernel execution boundries within sampling interval. */
        exec_start_cycles = atomic_load_local_64(&kernel->kw_cycles.exec_start_cycles);
        kw_tans_cycles = atomic_exchange_local_64(&kernel->kernel_exec_cycles, 0);
        exec_end_cycles = exec_start_cycles + kw_tans_cycles;

        /* The case where kernel execution has started or completed before or after sampling interval end,
           this needs to be skipped
          Kernel execution                   Kernel execution
            ───────┐                         ┌───────────
            ───────┴───▲─────────────────▲───┴───────────
                       │                 │
                Interval start          end
        */
        if ((exec_start_cycles > interval_end) && (kw_tans_cycles == 0))
        {
            continue;
        }

        /* Check if there is any active kernel execution. */
        if (atomic_load_local_32(&kernel->kernel_state) == KERNEL_STATE_IN_USE)
        {
            uint64_t temp_cycles = 0;
            bool cycles_exists = false;

            /* scenario where kernel execution started before sampling interval and ended within sampling interval.
            the execution cycles before start of sampling interval has to be subtracted from total execution cycles.
                              Kernel execution             Kernel execution(continued)
                            ┌───▲─────────┐   ▲          ┌───▲───────────────▲─┐
                            │   │         │   │          │   │               │ │
                         ───┴───┼─────────┴───┼──────────┴───┼───────────────┼─┴──────
                    Interval    │             │              │               │
                                start          end         start            end
            */
            if (exec_start_cycles < interval_start)
            {
                /* if kernel execution is continued past the sampling interval end then end cycles will be equal to start cycles
                because there are no total kernel execution cycles. In this case total cycles will be the interval cycles */
                temp_cycles += STATW_CHECK_FOR_CONTINUED_EXEC_TRANSACTION(interval_start,
                    interval_end, exec_start_cycles, exec_end_cycles, kw_tans_cycles,
                    (interval_start - exec_start_cycles), &kernel->kw_cycles.prev_cycles);
                cycles_exists = true;
            }
            else if ((exec_start_cycles < interval_end) && (exec_start_cycles > interval_start))
            {
                /* scenario where kernel execution started after sampling interval start and continued past interval end.
                            Kernel execution
                    ▲      ┌────────▲──────┐
                    │      │        │      │
                ────┼──────┴────────┼──────┴──
                    │               │
                    start            end
                */
                temp_cycles += interval_end - exec_start_cycles;
                atomic_add_local_64(
                    &kernel->kw_cycles.prev_cycles, (interval_end - exec_start_cycles));
                cycles_exists = true;
            }

            /* scenario where KW transaction was completed earlier and channel is also active for a new transaction */
            if (!((exec_start_cycles > interval_end) && (exec_end_cycles > interval_end)) &&
                (kw_tans_cycles > 0))
            {
                temp_cycles +=
                    kw_tans_cycles - atomic_exchange_local_64(&kernel->kw_cycles.prev_cycles, 0);
                /* add current accomodated cycles to previous cycles for this new transaction */
                atomic_add_local_64(
                    &kernel->kw_cycles.prev_cycles, (interval_end - exec_start_cycles));
                cycles_exists = true;
            }

            /* scenario where KW transaction was started and completed after interval and channel is also active for a new transaction */
            if (((exec_start_cycles > interval_end) && (exec_end_cycles > interval_end)) &&
                (kw_tans_cycles > 0))
            {
                atomic_exchange_local_64(&kernel->kw_cycles.prev_cycles, 0);
            }

            /* check if current calculated cycles exceeded interval cycles, accumulate reamining cycles into
            execution cycles to be addressed in next interval */
            if (temp_cycles > interval_cycles)
            {
                atomic_add_local_64(&kernel->kernel_exec_cycles, (temp_cycles - interval_cycles));
                temp_cycles -= (temp_cycles - interval_cycles);
            }

            accum_cycles += temp_cycles;

            /* check if we have accumulated cycles for this channel */
            if (cycles_exists)
            {
                active_kw++;
            }

            /* Apply scaling factor. */
            accum_cycles = (accum_cycles * get_set_bit_count(
                                               atomic_load_local_64(&kernel->kernel_shire_mask))) /
                           total_shire_count;
        }
        else
        {
            /* scenario where kernel execution was completed and channel was idle
                                ▲  ┌────────┐   ▲
                                │  │        │   │
                            ────┼──┴────────┴───┼───
                                │               │
                                start            end
             */
            if (kw_tans_cycles > 0)
            {
                kw_tans_cycles -= atomic_exchange_local_64(&kernel->kw_cycles.prev_cycles, 0);

                /* check if the dma cycles are within interval range, accumulated remaining cycles into execution cycles to be addressed in next interval*/
                if (kw_tans_cycles > interval_cycles)
                {
                    atomic_add_local_64(
                        &kernel->kernel_exec_cycles, (kw_tans_cycles - interval_cycles));
                    accum_cycles += interval_cycles;
                }
                else
                {
                    accum_cycles += kw_tans_cycles;
                }
                active_kw++;
            }
        }
    }

    return (active_kw > 0 ? (accum_cycles / (uint64_t)active_kw) : accum_cycles);
}

/************************************************************************
*
*   FUNCTION
*
*       KW_Get_Kernel_State
*
*   DESCRIPTION
*
*       This function checks kernel state either IDLE or BUSY
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint64_t    kernel state - IDLE or BUSY
*
***********************************************************************/
uint64_t KW_Get_Kernel_State(void)
{
    uint8_t kernel_state = MM_STATE_IDLE;

    for (uint8_t i = 0; i < MM_MAX_PARALLEL_KERNELS; i++)
    {
        /* loop through all kernels to check state */
        if (atomic_load_local_32(&KW_CB.kernels[i].kernel_state) == KERNEL_STATE_IN_USE)
        {
            kernel_state = MM_STATE_BUSY;
            break;
        }
    }

    return kernel_state;
}