#include "services/log.h"
#include "device-common/macros.h"

uint64_t exception_handler(uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t *const regs);

uint64_t exception_handler(uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t *const regs)
{
    (void) stval;
    (void) regs;

    /* TODO: SW-6573: Decode exception and reset Master FW */

    Log_Write(LOG_LEVEL_CRITICAL, "MasterMinon exception: unhandled exception: %lx\n", scause);

    C_TEST_FAIL

    /* TODO: return +2 if compressed instruction ... */
    return sepc + 4;
}
