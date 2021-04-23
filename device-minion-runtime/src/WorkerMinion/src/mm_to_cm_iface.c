#include "etsoc_memory.h"
#include "flb.h"
#include "cacheops.h"
#include "hart.h"
#include "kernel.h"
#include "layout.h"
#include "log.h"
#include "message_types.h"
#include "mm_to_cm_iface.h"
#include "riscv_encoding.h"
#include "sync.h"
#include "syscall_internal.h"

/* MM -> CM message buffers */
#define master_to_worker_broadcast_message_buffer_ptr \
    ((cm_iface_message_t *)FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER)
#define master_to_worker_broadcast_message_ctrl_ptr \
    ((broadcast_message_ctrl_t *)FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL)

static spinlock_t notify_local_barrier[NUM_SHIRES] = { 0 };

/* Local function prototypes */
static void mm_to_cm_iface_handle_message(uint32_t shire, uint64_t hart, cm_iface_message_t *const message_ptr);

/* Identify the last thread in pool */
static inline bool find_last_thread(spinlock_t *lock, uint32_t num_threads)
{
    if (atomic_add_local_32(&lock->flag, 1U) == (num_threads - 1))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/* This barrier is required to synchronize all Shires before handling the message */
static inline void synchronize_shires(uint64_t shire_id)
{
    const uint32_t thread_count = (shire_id == MASTER_SHIRE) ? 32 : 64;
    bool last;

    last = find_last_thread(&notify_local_barrier[shire_id], thread_count);

    /* Last thread per shire decrements global counter and waits for all Shires to reach sync point */
    if (last)
    {
        int32_t last_shire = atomic_add_signed_global_32(
            (int32_t*)&master_to_worker_broadcast_message_ctrl_ptr->shire_count, -1);

        /* Reset the local barrier flag */
        init_local_spinlock(&notify_local_barrier[shire_id], 0);

        /* Last shire sends IPI to the sender thread in MM */
        if(last_shire == 1)
        {
            /* Send IPI to the sender */
            syscall(SYSCALL_IPI_TRIGGER_INT,
                1ull << master_to_worker_broadcast_message_ctrl_ptr->sender_thread_id, MASTER_SHIRE, 0);
        }
    }

    /* All threads in Shire wait for Last Thread to clear flag */
    local_spinwait_wait(&notify_local_barrier[shire_id], 0);
}

void __attribute__((noreturn)) MM_To_CM_Iface_Main_Loop(void)
{
    for (;;)
    {
        /* Wait for an IPI (Software Interrupt) */
        asm volatile("wfi\n");

        /* We got a software interrupt (IPI) handed down from M-mode.
        M-mode already cleared the MSIP (Machine Software Interrupt Pending)
        Clear Supervisor Software Interrupt Pending (SSIP) */
        asm volatile("csrci sip, %0" : : "I"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

        /* Receive and process message buffer */
        MM_To_CM_Iface_Multicast_Receive();
    }
}

void MM_To_CM_Iface_Multicast_Receive(void)
{
    const uint32_t shire_id = get_shire_id();
    cm_iface_message_t message;

    asm volatile("fence");
    evict(to_L3, master_to_worker_broadcast_message_buffer_ptr, sizeof(message));
    WAIT_CACHEOPS;

    /* Copy message from shared global memory to local buffer */
    ETSOC_Memory_Read_Write_Cacheable(master_to_worker_broadcast_message_buffer_ptr,
                                      &message, sizeof(message));

    /* Synchronize all the shires to reach sync point and ack back to MM */
    synchronize_shires(shire_id);

    mm_to_cm_iface_handle_message(shire_id, get_hart_id(), &message);
}

static void mm_to_cm_iface_handle_message(uint32_t shire, uint64_t hart, cm_iface_message_t *const message_ptr)
{
    switch (message_ptr->header.id)
    {
    case MM_TO_CM_MESSAGE_ID_KERNEL_LAUNCH:
    {
        int64_t rv = -1;
        mm_to_cm_message_kernel_launch_t *launch = (mm_to_cm_message_kernel_launch_t *)message_ptr;
        /* Check if this Shire is involved in the kernel launch */
        if (launch->shire_mask & (1ULL << shire))
        {
            uint64_t kernel_stack_addr = KERNEL_UMODE_STACK_BASE - (hart * KERNEL_UMODE_STACK_SIZE);
            rv = launch_kernel(launch->kw_base_id, launch->slot_index, launch->code_start_address, kernel_stack_addr,
                               launch->pointer_to_args, launch->flags, launch->shire_mask);
        }

        if (rv != 0)
        {
            // Something went wrong launching the kernel.
            // TODO: Do something
        }
        break;
    }
    case MM_TO_CM_MESSAGE_ID_KERNEL_ABORT:
    {
        /* Should only abort if the kernel was launched on this hart */
        if (kernel_info_has_thread_launched(shire, hart & (HARTS_PER_SHIRE - 1)))
        {
            return_from_kernel(KERNEL_LAUNCH_ERROR_ABORTED);
        }
        break;
    }
    case MM_TO_CM_MESSAGE_ID_SET_LOG_LEVEL:
        log_set_level(((mm_to_cm_message_set_log_level_t *)message_ptr)->log_level);
        break;
    case MM_TO_CM_MESSAGE_ID_TRACE_UPDATE_CONTROL:
        // Evict to invalidate control region to get new changes
        // TODO: Implement new Tracing
        // TRACE_update_control();
        break;
    case MM_TO_CM_MESSAGE_ID_TRACE_BUFFER_RESET:
        // Reset trace buffer for next run
        // TODO: Implement new Tracing
        // TRACE_init_buffer();
        break;
    case MM_TO_CM_MESSAGE_ID_TRACE_BUFFER_EVICT:
        // Evict trace buffer for consumption
        // TODO: Implement new Tracing
        // TRACE_evict_buffer();
        break;
    case MM_TO_CM_MESSAGE_ID_PMC_CONFIGURE:
        // Make a syscall to M-mode to configure PMCs
        syscall(SYSCALL_CONFIGURE_PMCS_INT, 0,
                ((mm_to_cm_message_pmc_configure_t *)message_ptr)->conf_buffer_addr, 0);
        break;
    default:
        // Unknown message
        break;
    }
}
