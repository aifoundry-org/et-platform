#include "atomic.h"
#include "device-mrt-trace.h"
#include "hart.h"
#include "kernel.h"
#include "layout.h"
#include "log.h"
#include "message.h"
#include "syscall_internal.h"

static message_number_t previous_broadcast_message_number[NUM_HARTS]
    __attribute__((section(".data"))) = { 0 };

void swi_handler(void);
static void handle_message(uint64_t shire, uint64_t hart, message_t *const message_ptr);

// Must not access kernel_config - firmware assumes kernel_config addresses remain clean/invalid until reading on a FCC
void swi_handler(void)
{
    message_t message;

    // We got a software interrupt (IPI) handed down from M-mode.
    // M-mode already cleared the MSIP (IPI) - check messages

    // Clear pending software interrupt (SSIP)
    asm volatile("csrci sip, 0x2");

    const uint64_t shire = get_shire_id();
    const uint64_t hart = get_hart_id();

    volatile uint8_t *addr = &previous_broadcast_message_number[hart];
    if (broadcast_message_available(atomic_load_global_8(addr))) {
        message_number_t number = broadcast_message_receive_worker(&message);
        atomic_store_global_8(addr, number);
        handle_message(shire, hart, &message);
    }

    if (message_available(shire, hart)) {
        message_receive_worker(shire, hart, &message);
        handle_message(shire, hart, &message);
    }
}

static void handle_message(uint64_t shire, uint64_t hart, message_t *const message_ptr)
{
    switch (message_ptr->header.id) {
    case MESSAGE_ID_KERNEL_ABORT:
        // If kernel was running, returns to firmware context. If not, doesn't do anything.
        return_from_kernel(KERNEL_LAUNCH_ERROR_ABORTED);
        break;
    case MESSAGE_ID_SET_LOG_LEVEL:
        log_set_level(((message_set_log_level_t *)message_ptr)->log_level);
        break;
    case MESSAGE_ID_LOOPBACK:
        message_send_worker(shire, hart, message_ptr);
        break;
    case MESSAGE_ID_TRACE_UPDATE_CONTROL:
        // Evict to invalidate control region to get new changes
        TRACE_update_control();
        break;
    case MESSAGE_ID_TRACE_BUFFER_RESET:
        // Reset trace buffer for next run
        TRACE_init_buffer();
        break;
    case MESSAGE_ID_TRACE_BUFFER_EVICT:
        // Evict trace buffer for consumption
        TRACE_evict_buffer();
        break;
    case MESSAGE_ID_PMC_CONFIGURE:
        // Make a syscall to M-mode to configure PMCs
        syscall(SYSCALL_CONFIGURE_PMCS_INT, 1,
                ((message_pmc_configure_t *)message_ptr)->conf_buffer_addr, 0);
        break;
    default:
        // Unknown message
        break;
    }
}
