#include "ipi.h"
#include "message.h"
#include "printf.h"
#include "print_exception.h"
#include "shire.h"

#include <inttypes.h>

void swi_handler(void);

void swi_handler(void)
{
    // We got a software interrupt handed down from M-mode.
    // M-mode already cleared the IPI - Go check mailboxes

    //Check for messages from worker minion
    for (uint64_t shire = 0; shire < 33; shire++)
    {
        const uint64_t flags = get_message_flags(shire);

        if (flags != 0)
        {
            for (uint64_t hart = 0; hart < 64; hart++)
            {
                if (flags & (1ULL << hart))
                {
                    message_t message;

                    message_receive_master(shire, hart, &message);

                    if (message.id == MESSAGE_ID_EXCEPTION)
                    {
                        print_exception(message.data[1],
                                        message.data[2],
                                        message.data[3],
                                        message.data[4],
                                        message.data[0]);
                    }
                    else if (message.id == 0xDEADBEEF)
                    {
                        //printf("reflected message received, data[0] = 0x%016" PRIx64 "\r\n", message.data[0]);
                    }
                    else
                    {
                        printf("unknown message received, id = 0x%016" PRIx64 " data[0] = 0x%016" PRIx64 "\r\n", message.id, message.data[0]);
                    }

                    // Ensure the reads from message complete before the flag is cleared
                    asm volatile ("fence");
                }
            }
        }
    }

    // Clear pending software interrupt
    asm volatile ("csrci sip, 0x2");
}
