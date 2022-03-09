#include <etsoc/isa/hart.h>
#include <etsoc/isa/riscv_encoding.h>
#include <system/layout.h>
#include "cm_to_mm_iface.h"
#include "kernel.h"
#include "log.h"
#include "mm_to_cm_iface.h"

void swi_handler(uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t *const reg);

/* If we are here it means we were executing Userspace code,
because in S-mode, IPIs (Supervisor Software Interrupts) are masked */
void swi_handler(uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t *const reg)
{
    /* We got a software interrupt (IPI) handed down from M-mode.
    M-mode already cleared the MSIP (Machine Software Interrupt Pending)
    Clear Supervisor Software Interrupt Pending (SSIP) */
    asm volatile("csrci sip, %0" : : "I"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

    /* Save the execution context so that it may be used in abort cases */
    internal_execution_context_t context = { .scause = scause,
        .sepc = sepc,
        .stval = stval,
        .regs = reg };

    Log_Write(LOG_LEVEL_DEBUG, "swi_handler:IPI received:handling MM msg\r\n");

    /* Handle messages from MM */
    MM_To_CM_Iface_Multicast_Receive((void *)&context);
}
