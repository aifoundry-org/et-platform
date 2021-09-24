/* mm_rt_svcs */
#include "etsoc/isa/macros.h"

/* mm specific headers */
#include "services/log.h"
#include "services/sp_iface.h"
#include "services/trace.h"

uint64_t exception_handler(uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t *const regs);

uint64_t exception_handler(uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t *const regs)
{
    uint64_t sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));

    struct dev_context_registers_t context = {.epc = sepc, .tval = stval,
        .status = sstatus, .cause = scause};

    /* Copy all the GPRs except x0 (hardwired to zero) */
    memcpy(context.gpr, &regs[1], sizeof(uint64_t) * TRACE_DEV_CONTEXT_GPRS);

    /* Log the execution stack event to trace */
    Trace_Execution_Stack(Trace_Get_MM_CB(), &context);

    Log_Write(LOG_LEVEL_CRITICAL,
        "MasterMinon: S-mode exception: scause: %lx sepc: %lx stval: %lx sstatus: %lx\n",
        scause, sepc, stval, sstatus);

    /* Report SP of unrecoverable error. SP will reset MM. */
    SP_Iface_Report_Error(SP_RECOVERABLE, MM_RUNTIME_EXCEPTION);

    C_TEST_FAIL

    /* TODO: return +2 if compressed instruction ... */
    return sepc + 4;
}
