#include "cacheops.h"
#include "device-mrt-trace.h"
#include "fcc.h"
#include "flb.h"
#include "hart.h"
#include "kernel.h"
#include "layout.h"
#include "message.h"
#include "mm_iface.h"
#include "sync.h"
#include "syscall_internal.h"

#include <stdint.h>

static cm_iface_message_number_t previous_broadcast_message_number[NUM_HARTS]
    __attribute__((aligned(64))) = { 0 };

static void handle_message_from_mm(uint64_t shire, uint64_t hart, cm_iface_message_t *const message_ptr);

void mm_iface_process(void)
{
    cm_iface_message_t message;
    const uint64_t shire_id = get_shire_id();
    const uint64_t hart_id = get_hart_id();

    volatile uint8_t *addr = &previous_broadcast_message_number[hart_id];
    if (broadcast_message_available_worker(atomic_load_global_8(addr))) {
        cm_iface_message_number_t number = broadcast_message_receive_worker(&message);
        atomic_store_global_8(addr, number);
        handle_message_from_mm(shire_id, hart_id, &message);
    }

    if (message_available_worker(shire_id, hart_id)) {
        message_receive_worker(shire_id, hart_id, &message);
        handle_message_from_mm(shire_id, hart_id, &message);
    }
}

static void handle_message_from_mm(uint64_t shire, uint64_t hart, cm_iface_message_t *const message_ptr)
{
    switch (message_ptr->header.id) {
    case MM_TO_CM_MESSAGE_ID_KERNEL_LAUNCH: {
        int64_t rv = -1;
        mm_to_cm_message_kernel_launch_t *launch = (mm_to_cm_message_kernel_launch_t *)message_ptr;
        for (uint64_t i = 0; i < MAX_SIMULTANEOUS_KERNELS; i++) {
            if (launch->shire_mask & (1ULL << shire)) {
                uint64_t kernel_stack_addr = KERNEL_UMODE_STACK_BASE - (hart * KERNEL_UMODE_STACK_SIZE);

                rv = launch_kernel(launch->kw_id, launch->code_start_address, kernel_stack_addr,
                                   launch->pointer_to_args, launch->flags, launch->shire_mask);
                break;
            }
        }

        if (rv != 0) {
            // Something went wrong launching the kernel.
            // TODO: Do something
        }
        break;
    }
    case MM_TO_CM_MESSAGE_ID_KERNEL_ABORT:
        // If kernel was running, returns to firmware context. If not, doesn't do anything.
        return_from_kernel(KERNEL_LAUNCH_ERROR_ABORTED);
        break;
    case MM_TO_CM_MESSAGE_ID_SET_LOG_LEVEL:
        log_set_level(((mm_to_cm_message_set_log_level_t *)message_ptr)->log_level);
        break;
    case MM_TO_CM_MESSAGE_ID_TRACE_UPDATE_CONTROL:
        // Evict to invalidate control region to get new changes
        TRACE_update_control();
        break;
    case MM_TO_CM_MESSAGE_ID_TRACE_BUFFER_RESET:
        // Reset trace buffer for next run
        TRACE_init_buffer();
        break;
    case MM_TO_CM_MESSAGE_ID_TRACE_BUFFER_EVICT:
        // Evict trace buffer for consumption
        TRACE_evict_buffer();
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
