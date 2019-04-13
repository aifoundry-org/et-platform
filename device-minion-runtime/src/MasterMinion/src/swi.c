#include "ipi.h"
#include "shire.h"

void swi_handler(void);

void swi_handler(void)
{
    // We got a software interrupt. Is it from PCI-E? Is it from a minion?
    // Go read the mailboxes and find out.

    // Clear the IPI first: any interrupts that arrived by now had their
    // messages placed in memory fist. Any interrupts that arrive after
    // here will be handled on a subsequent IPI.
    IPI_TRIGGER_CLEAR(get_shire_id(), 1U << (get_hart_id() % 64));

    // Check R_PU_MBOX_MM_SP @ 0x0020006000

    // R_PU_MBOX_PC_MM @ 0x0020007000

    // Worker minions: do we seriously have to poll 2048 mailboxes?

    // FIXME What if one source sends a IPI_TRIGGER, we handle it,
    // and the other source sets an IPI_TRIGGER after we handle but
    // before we clear? Won't we lose that interrupt?
}
