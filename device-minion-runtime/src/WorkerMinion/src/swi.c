#include "kernel.h"
#include "message.h"
#include "shire.h"

void swi_handler(void);

void swi_handler(void)
{
    // We got a software interrupt handed down from M-mode.
    // M-mode already cleared the IPI - check messages

    const uint64_t shire = get_shire_id();
    const uint64_t hart = get_hart_id();
    const uint64_t message_id = get_message_id(shire, hart);

    if (message_id == MESSAGE_ID_KERNEL_ABORT)
    {
        // If kernel was running, returns to firmware context
        // If not, doesn't do anything.
        return_from_kernel();
    }

    // Otherwise handle the message in firmware context

    // Clear pending software interrupt
    asm volatile ("csrci sip, 0x2");
}
