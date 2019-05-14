#include "exception.h"
#include "esr_defines.h"
#include "message.h"
#include "shire.h"
#include "sync.h"

#include <inttypes.h>

void swi_handler(void);

void swi_handler(void)
{
    const uint64_t hart_id = get_hart_id();

    // Clear the IPI first: any interrupts that arrived by now had their
    // messages placed in memory first. Any interrupts that arrive after
    // here will be handled on a subsequent IPI.
    volatile uint64_t* const ipi_trigger_clear_ptr = ESR_SHIRE(PRV_M, 0xFF, IPI_TRIGGER_CLEAR);
    *ipi_trigger_clear_ptr = 1ULL << (hart_id % 64);

    // Ensure the write to clear happens before any message reads
    asm volatile ("fence");

    // Forward to supervisor mode
    asm volatile ("csrsi mip, 0x2");
}
