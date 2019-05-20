#include "master.h"
#include "interrupt.h"
#include "ipi.h"
#include "message.h"
#include "net_desc.h"
#include "printf.h"
#include "serial.h"
#include "shire.h"
#include "sync.h"
#include "syscall.h"

#include <inttypes.h>

#define NUM_WORKER_SHIRES 1

void MASTER_thread(void)
{
    // TODO FIXME hangs on RVTimer access, waiting for RTLMIN-2778
    //volatile const uint64_t * const pullTime = (volatile const uint64_t * const)(0x01FF800000 + 0x0);

    if (get_hart_id() != 2048)
    {
        asm volatile ("wfi");
    }

    SERIAL_init(UART0);
    printf("\r\nmaster minion\r\n");

    message_init_master();

    //const uint64_t before = *pullTime;

    // TODO FIXME convenient counter location for zebu inspection
    register volatile uint64_t masterCycles asm("t0") = 0;

    while (1)
    {
        // Wait for FCC1 from each worker shire indicating firmware is ready for a new kernel
        for (unsigned int i = 0; i < NUM_WORKER_SHIRES; i++)
        {
            WAIT_FCC(1);
        }

        printf("%" PRIu64 "\r\n", masterCycles);

        message_t message = {.id = 0xDEADBEEF, .data = {0,0,0,0,0,0,0}};
        message_send_master(0, masterCycles % 32U, &message);

        // Parse flatbuffer network description here

        // Send kernel entry PC to all worker minon here

        // Send every worker thread a FCC_1 to start the kernel
        // TODO FIXME or just send to 1 hart per minion and have it relay if that's faster
        // Prefetch threads will start loading instructions for other threads
        // Other threads will eventually receive this credit to know they can start,
        // but will need to wait for subsequent data motion credits before doing any work.
        for (unsigned int i = 0; i < NUM_WORKER_SHIRES; i++)
        {
            SEND_FCC(i, THREAD_0, FCC_1, 0xFFFFFFFFU);
            SEND_FCC(i, THREAD_1, FCC_1, 0xFFFFFFFFU);
        }

        // // Start with some credits to store tensor ops of data in memory
        // SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, 1);
        // SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, 1);
        // SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, 1);
        // SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, 1);

        // // Blast of credits for now, replace with credits every x KB/MB per network description
        // for (unsigned int cycles = 0; cycles < 0xFFFF; cycles++)
        // {
        //     // PCI-E DMA (one tensor op? entire layer's worth?) of data from Host to memory here
        //     // Before PCI-E is up, parsing of MM-API flatbuffers can be inserted here for testing.

        //     // Send every shire 0 prefetch thread (thread 1) a FCC1 credit indicating one tensor op? an entire layer? of data in memory is ready
        //     // TODO FIXME open questions on master->prefetch credits:
        //     // - one FCC when entire layer's A/B data is in memory? Adds a bunch of startup latency.
        //     // - fine grain credits for Each tensor op's worth of A/B is ready?
        //     // - something in between?
        //     SEND_FCC(0, THREAD_1, FCC_1, 1);
        // }

        //Wait for a FCC1 from each worker shire indicating kernel is complete
        for (unsigned int i = 0; i < NUM_WORKER_SHIRES; i++)
        {
            WAIT_FCC(1);
        }

        //register const uint64_t delta = *pullTime - before;

        masterCycles++;
    }

    //asm volatile ("wfi");
}
