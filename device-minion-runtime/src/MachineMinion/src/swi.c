#include "ipi.h"
#include "shire.h"

void swi_handler(void);

void swi_handler(void)
{
    // Clear the IPI first: any interrupts that arrived by now had their
    // messages placed in memory fist. Any interrupts that arrive after
    // here will be handled on a subsequent IPI.
    IPI_TRIGGER_CLEAR(get_shire_id(), 1U << (get_hart_id() % 64));

    // Ensure the write to clear happens before any mailbox reads
    asm volatile ("fence");

    // Forward to supervisor mode
    asm volatile ("csrsi mip, 0x2");
}
