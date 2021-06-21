#include "cm_to_mm_iface.h"
#include "device-common/hart.h"
#include "kernel.h"
#include "layout.h"
#include "mm_to_cm_iface.h"
#include "riscv_encoding.h"

void swi_handler(uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t *const reg);

/* If we are here it means we were executing Userspace code,
because in S-mode, IPIs (Supervisor Software Interrupts) are masked */
void swi_handler(uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t *const reg)
{
    /* We got a software interrupt (IPI) handed down from M-mode.
    M-mode already cleared the MSIP (Machine Software Interrupt Pending)
    Clear Supervisor Software Interrupt Pending (SSIP) */
    asm volatile("csrci sip, %0" : : "I"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

    uint32_t shire_id = get_shire_id();

    /* Check for kernel abort handled down by a hart */
    if(kernel_info_get_abort_flag(shire_id) == 1)
    {
        uint64_t sstatus;
        asm volatile("csrr %0, sstatus" : "=r"(sstatus));

        /* Get the kernel exception buffer */
        uint64_t exception_buffer = kernel_info_get_exception_buffer(shire_id);

        /* If the kernel exception buffer is available */
        if(exception_buffer != 0)
        {
            swi_execution_context_t context = {.scause = scause, .sepc = sepc,
                .sstatus = sstatus, .stval = stval, .regs = reg};

            /* Save the execution context in the buffer provided (system abort case) */
            CM_To_MM_Save_Execution_Context((execution_context_t*)exception_buffer,
                CM_CONTEXT_TYPE_SYSTEM_ABORT, get_hart_id(), &context);
        }

        return_from_kernel(KERNEL_LAUNCH_ERROR_ABORTED);
    }
    else
    {
        /* Save the execution context so that it may be used in abort cases */
        swi_execution_context_t context = {.scause = scause, .sepc = sepc, .stval = stval, .regs = reg};

        /* Handle messages from MM */
        MM_To_CM_Iface_Multicast_Receive((void*)&context);
    }
}


