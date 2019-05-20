#include "ipi.h"
#include "message.h"
#include "shire.h"
#include "syscall.h"

void swi_handler(void);

void swi_handler(void)
{
    // We got a software interrupt handed down from M-mode.
    // M-mode already cleared the IPI - Go check messages

    const uint64_t shire = get_shire_id();
    const uint64_t hart = get_hart_id();

    if (get_message_id(shire, hart) != 0U)
    {
        message_t message;

        message_receive_worker(shire, hart, &message);

        // TODO FIXME HACK reflect test message back to master
        if (message.id == 0xDEADBEEF)
        {
            message.data[0] = hart;
            message_send_worker(shire, hart, &message);
        }

        // if (message.id == MESSAGE_ID_KERNEL_LAUNCH)
        // {

        // }

        // if (message.id == MESSAGE_ID_ABORT)
        // {

        // }
    }

    // Clear pending software interrupt
    asm volatile ("csrci sip, 0x2");
}
