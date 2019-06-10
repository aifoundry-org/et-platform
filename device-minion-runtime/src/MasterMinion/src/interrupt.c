#include "interrupt.h"

static void dummyIsr(void);
static void pcieIsr(void);

void (*vectorTable[PU_PLIC_INTR_CNT])(void) =
{
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    dummyIsr,
    pcieIsr
};

void* pullVectorTable = vectorTable;

static void dummyIsr(void)
{
    // TODO FIXME @Will when the host sends an interrupt, atomically set/increment a host interrupt flag/counter
}

static void pcieIsr(void)
{
    // TODO FIXME @Will when PCI-E sends a data transfer completion interrupt
    // update kernel state if needed (e.g. DONE -> NONE after all results are sent to host)
    // Can send events from ISR to be handled in main wfi loop - safer to update state with deterministic ordering
}
