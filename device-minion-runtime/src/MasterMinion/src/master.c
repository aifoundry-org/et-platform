#include "master.h"
#include "interrupt.h"
#include "ipi.h"
#include "net_desc.h"
#include "shire.h"
#include "sync.h"

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

	// Get data from net description
	// uint64_t    count    = (* (uint64_t *) 0x8200000000ULL);
	// net_desc_t *net_desc = (net_desc_t*) 0x8200000040ULL;

    //const uint64_t before = *pullTime;


    // For the Master Minion, the interrupts will be triggered with the general-purpose IPI register
    // from the Esperanto RISV-V IPI Extension. Writes to this register will cause a software trap
    // on the Master Minion (MSIP bit).

    // Sending ourselves an IPI works: swi_handler is called and we return
    IPI_TRIGGER(THIS_SHIRE, 1U);

    // Start with some credits to store tensor ops of data in memory
    SEND_FCC(0, THREAD_0, FCC_0, 1);
    SEND_FCC(0, THREAD_0, FCC_0, 1);
    SEND_FCC(0, THREAD_0, FCC_0, 1);
    SEND_FCC(0, THREAD_0, FCC_0, 1);

    // Do a bunch, but don't wrap the 16 bit fast credit counter value
    for (unsigned int cycles = 0; cycles < 0xFFFF; cycles++)
    {
        // PCI-E DMA (one tensor op? entire layer's worth?) of data from Host to memory here
        // Before PCI-E is up, parsing of MM-API flatbuffers can be inserted here for testing.

        // Send every shire 0 prefetch thread (thread 1) a FCC1 credit indicating one tensor op? an entire layer? of data in memory is ready
        // TODO FIXME open questions on master->prefetch credits:
        // - one FCC when entire layer's A/B data is in memory? Adds a bunch of startup latency.
        // - fine grain credits for Each tensor op's worth of A/B is ready?
        // - something in between?
        SEND_FCC(0, THREAD_1, FCC_1, 1);
    }

    // Wait for a FCC0 from compute thread indicating kernel is complete
    //WAIT_FCC(0);

    // TODO Send post-kernel cleanup IPI

    //register const uint64_t delta = *pullTime - before;

    asm volatile ("wfi");
}
