#include "master.h"
#include "interrupt.h"
#include "ipi.h"
#include "net_desc.h"
#include "shire.h"
#include "sync.h"

#define NUM_WORKER_SHIRES 1

void MASTER_thread(void)
{
    unsigned int temp;

    // Set MIE.MEIE and MIE.MSIE to enable machine external and software interrupts
    asm volatile ("li %0, 0x808\n"
                  "csrs mie, %0\n" : "=&r" (temp));

    // Enable global interrupts
    INT_enableInterrupts();

    // TODO FIXME hangs on UART access, waiting for RTLMIN-2778
    //SERIAL_init(UART0);
    //SERIAL_write(UART0, "alive\r\n", 7);

    // TODO FIXME hangs on RVTimer access, waiting for RTLMIN-2778
    //volatile const uint64_t * const pullTime = (volatile const uint64_t * const)(0x01FF800000 + 0x0);

    if (get_minion_id() != 1024)
    {
        asm volatile ("wfi");
        while (1) {} // in case wfi isn't working
    }

    //const uint64_t before = *pullTime;

    // Wait for FCC1 from each worker shire indicating firmware has started
    for (unsigned int i = 0; i < NUM_WORKER_SHIRES; i++)
    {
        WAIT_FCC(1);
    }

    // Sending ourselves an IPI works: swi_handler is called and we return
    // This is how PCI-E notifications of writes from the host will arrive, so faking for now
    IPI_TRIGGER(THIS_SHIRE, 1U);

	// Parse flatbuffer network description here

    // Send kernel entry PC to all worker minon here

    // Send every worker thread a FCC_1 to start the kernel
    // Prefetch threads will start loading instructions for other threads
    // Other threads will eventually receive this credit to know they can start,
    // but will need to wait for subsequent data motion credits before doing any work.
    for (unsigned int i = 0; i < NUM_WORKER_SHIRES; i++)
    {
        SEND_FCC(i, THREAD_0, FCC_1, 0xFFFFFFFFU);
        SEND_FCC(i, THREAD_1, FCC_1, 0xFFFFFFFFU);
    }

    // Start with some credits to store tensor ops of data in memory
    SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, 1);
    SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, 1);
    SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, 1);
    SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, 1);

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

    // TODO Send post-kernel cleanup IPI

    //register const uint64_t delta = *pullTime - before;

    asm volatile ("wfi");
}
