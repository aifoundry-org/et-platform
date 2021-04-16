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


/* MM to CM interface */
static cm_iface_message_number_t g_previous_broadcast_message_number[NUM_HARTS] __attribute__((aligned(64))) = { 0 };

// master -> worker
#define master_to_worker_broadcast_message_buffer_ptr \
    ((cm_iface_message_t *)FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER)
#define master_to_worker_broadcast_message_ctrl_ptr \
    ((broadcast_message_ctrl_t *)FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL)

static void MM_To_CM_Iface_Handle_Message(uint64_t shire, uint64_t hart, cm_iface_message_t *const message_ptr);
static bool MM_To_CM_Iface_Multicast_Receive_Message_Available(cm_iface_message_number_t previous_broadcast_message_number);
static cm_iface_message_number_t MM_To_CM_Iface_Multicast_Receive(cm_iface_message_t *const message);

void __attribute__((noreturn)) MM_To_CM_Iface_Main_Loop(void)
{
    for (;;) {
        // Wait for an IPI (Software Interrupt)
        asm volatile("wfi\n");

        // We got a software interrupt (IPI) handed down from M-mode.
        // M-mode already cleared the MSIP (Machine Software Interrupt Pending)
        // Clear Supervisor Software Interrupt Pending (SSIP)
        asm volatile("csrci sip, %0" : : "I"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

        // Handle messages from MM
        MM_To_CM_Iface_Process();
    }
}

void MM_To_CM_Iface_Process(void)
{
    cm_iface_message_t message;
    const uint64_t shire_id = get_shire_id();
    const uint64_t hart_id = get_hart_id();

    volatile uint8_t *addr = &g_previous_broadcast_message_number[hart_id];
    if (MM_To_CM_Iface_Multicast_Receive_Message_Available(atomic_load_global_8(addr))) {
        cm_iface_message_number_t number = MM_To_CM_Iface_Multicast_Receive(&message);
        atomic_store_global_8(addr, number);
        MM_To_CM_Iface_Handle_Message(shire_id, hart_id, &message);
    }
}

// returns true if the broadcast message id != the previously received broadcast message
bool MM_To_CM_Iface_Multicast_Receive_Message_Available(cm_iface_message_number_t previous_broadcast_message_number)
{
    cm_iface_message_number_t cur_number =
        atomic_load_global_8(&master_to_worker_broadcast_message_buffer_ptr->header.number);

    return (cur_number != previous_broadcast_message_number);
}

cm_iface_message_number_t MM_To_CM_Iface_Multicast_Receive(cm_iface_message_t *const message)
{
    bool last;
    uint32_t thread_count = (get_shire_id() == MASTER_SHIRE) ? 32 : 64;

    asm volatile("fence");
    evict(to_L3,master_to_worker_broadcast_message_buffer_ptr,sizeof(*message));
    WAIT_CACHEOPS; 

    /* Copy message from shared global memory to local buffer */
    ETSOC_Memory_Read_Write_Cacheable(master_to_worker_broadcast_message_buffer_ptr,
                                    message, sizeof(*message));

    // Check if we are the last receiver of the Shire
    // TODO: FLBs are not safe and FLB 31 might be used by the caller. Use *local* atomics instead
    WAIT_FLB(thread_count, 31, last);

    // If we are the last receiver of the Shire, notify MT we have received the message
    if (last)
    {
        atomic_add_global_32(&master_to_worker_broadcast_message_ctrl_ptr->count, 1);
    }

    return message->header.number;
}

static void MM_To_CM_Iface_Handle_Message(uint64_t shire, uint64_t hart, cm_iface_message_t *const message_ptr)
{
    switch (message_ptr->header.id) {
    case MM_TO_CM_MESSAGE_ID_KERNEL_LAUNCH: {
        mm_to_cm_message_kernel_launch_t *launch = (mm_to_cm_message_kernel_launch_t *)message_ptr;
        /* Check if this Shire is involved in the kernel launch */
        if (launch->shire_mask & (1ULL << shire)) {
            uint64_t kernel_stack_addr = KERNEL_UMODE_STACK_BASE - (hart * KERNEL_UMODE_STACK_SIZE);
            /* Does not return */
            launch_kernel(launch->kw_base_id, launch->slot_index, launch->code_start_address, kernel_stack_addr,
                          launch->pointer_to_args, launch->flags, launch->shire_mask);
        }
        break;
    }
    case MM_TO_CM_MESSAGE_ID_KERNEL_ABORT:
        /* If the thread has not completed yet, handle post-launch cleanup, if it has completed, ignore the abort. */
        if (!kernel_info_has_thread_completed((uint32_t)shire, hart & 63))
        {
            return_from_kernel(KERNEL_LAUNCH_ERROR_ABORTED);
        }
        break;
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
