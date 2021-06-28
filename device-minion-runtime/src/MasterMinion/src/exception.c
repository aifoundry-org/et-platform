#include "services/log.h"
#include "services/sp_iface.h"
#include "device-common/macros.h"

uint64_t exception_handler(uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t *const regs);

uint64_t exception_handler(uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t *const regs)
{
    (void) regs;

    Log_Write(LOG_LEVEL_CRITICAL,
        "MasterMinon: Unhandled exception: cause:%lx: sepc: %lx: stval: %lx\n", scause, sepc, stval);

    SP_Iface_Report_Error(SP_RECOVERABLE, MM_UNHANDLED_EXCEPTION);

    C_TEST_FAIL

    /* TODO: return +2 if compressed instruction ... */
    return sepc + 4;
}
