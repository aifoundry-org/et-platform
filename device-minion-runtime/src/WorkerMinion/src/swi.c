#include "kernel.h"
#include "hart.h"
#include "layout.h"
#include "log.h"
#include "message.h"

static message_number_t previous_broadcast_message_number[NUM_HARTS]
    __attribute__((section(".data"))) = { 0 };

void swi_handler(void);
static void handle_message(uint64_t shire, uint64_t hart, message_t *const message_ptr);

// Must not access kernel_config - firmware assumes kernel_config addresses remain clean/invalid until reading on a FCC
void swi_handler(void)
{
    message_t message;

    // We got a software interrupt handed down from M-mode.
    // M-mode already cleared the IPI - check messages
    const uint64_t shire = get_shire_id();
    const uint64_t hart = get_hart_id();

    if (broadcast_message_available(previous_broadcast_message_number[hart])) {
        previous_broadcast_message_number[hart] = broadcast_message_receive_worker(&message);
        handle_message(shire, hart, &message);
    }

    if (message_available(shire, hart)) {
        message_receive_worker(shire, hart, &message);
        handle_message(shire, hart, &message);
    }

    // Clear pending software interrupt
    asm volatile("csrci sip, 0x2");
}

static void handle_message(uint64_t shire, uint64_t hart, message_t *const message_ptr)
{
    if (message_ptr->id == MESSAGE_ID_KERNEL_ABORT) {
        // If kernel was running, returns to firmware context
        // If not, doesn't do anything.
        return_from_kernel(KERNEL_LAUNCH_ERROR_ABORTED);
    } else if (message_ptr->id == MESSAGE_ID_SET_LOG_LEVEL) {
        log_set_level(message_ptr->data[0]);
    } else if (message_ptr->id == MESSAGE_ID_LOOPBACK) {
        message_send_worker(shire, hart, message_ptr);
    }
    else if (message_ptr->id == MESSAGE_ID_UPDATE_TRACE_CONTROL)
    {
        // Evict to invalidate control region to get new changes
        evict_trace_control();
    }
}
