#include <inttypes.h>

#include <etsoc/isa/hart.h>
#include <etsoc/isa/riscv_encoding.h>

#include "mm_to_cm_iface.h"
#include "cm_to_mm_iface.h"
#include "kernel.h"
#include "kernel_return.h"
#include "log.h"

/* Restores firmware context and resumes execution in launch_kernel()
Called from machine context by M-mode trap handler (e.g. if the kernel takes an exception)
and by firmware (e.g. normal kernel completion, kernel abort) */
int64_t return_from_kernel(int64_t return_value, uint64_t return_type)
{
    const uint64_t thread_id = get_hart_id() & 63;
    const uint32_t shire_id = get_shire_id();
    uint64_t prev_returned = kernel_info_set_thread_returned(shire_id, thread_id);
    (void)return_type; /* To avoid compiler warning */

    /* A Hart in specific kernel launch can only pass through return_from_kernel once */
    if (!((prev_returned >> thread_id) & 1))
    {
        uint64_t *firmware_sp;

        /* Reset the launched bit for the current thread */
        kernel_info_reset_launched_thread(shire_id, thread_id);

        asm volatile("csrr  %0, sscratch \n"
                     "addi  %0, %0, 8    \n"
                     : "=r"(firmware_sp));

        if (*firmware_sp != 0)
        {
            // Switch to firmware stack
            // restore context from stack
            asm volatile("li    x1, 0x120        \n"  // bitmask to set sstatus SPP and SPIE
                         "csrs  sstatus, x1       \n" // set sstatus SPP and SPIE
                         "ld    sp, %0            \n" // load sp from supervisor stack SP region
                         "sd    zero, %0          \n" // clear supervisor stack SP region
                         "mv    x10, %[return_value]\n"
                         "mv    x11, %[return_type] \n" // use a1 to save the return type
                         "ld    x1,  1  * 8( sp ) \n"   // restore context
                         "ld    x3,  3  * 8( sp ) \n"
                         "ld    x4,  4  * 8( sp ) \n"
                         "ld    x5,  5  * 8( sp ) \n"
                         "ld    x6,  6  * 8( sp ) \n"
                         "ld    x7,  7  * 8( sp ) \n"
                         "ld    x8,  8  * 8( sp ) \n"
                         "ld    x9,  9  * 8( sp ) \n"
                         "ld    x12, 12 * 8( sp ) \n"
                         "ld    x13, 13 * 8( sp ) \n"
                         "ld    x14, 14 * 8( sp ) \n"
                         "ld    x15, 15 * 8( sp ) \n"
                         "ld    x16, 16 * 8( sp ) \n"
                         "ld    x17, 17 * 8( sp ) \n"
                         "ld    x18, 18 * 8( sp ) \n"
                         "ld    x19, 19 * 8( sp ) \n"
                         "ld    x20, 20 * 8( sp ) \n"
                         "ld    x21, 21 * 8( sp ) \n"
                         "ld    x22, 22 * 8( sp ) \n"
                         "ld    x23, 23 * 8( sp ) \n"
                         "ld    x24, 24 * 8( sp ) \n"
                         "ld    x25, 25 * 8( sp ) \n"
                         "ld    x26, 26 * 8( sp ) \n"
                         "ld    x27, 27 * 8( sp ) \n"
                         "ld    x28, 28 * 8( sp ) \n"
                         "ld    x29, 29 * 8( sp ) \n"
                         "ld    x30, 30 * 8( sp ) \n"
                         "ld    x31, 31 * 8( sp ) \n"
                         "addi  sp, sp, (32 * 8)  \n"
                         "ret                     \n"
                         : "+m"(*firmware_sp)
                         : [return_value] "r"(return_value), [return_type] "r"(return_type)
                         : "x10", "x11");

            return return_value;
        }
        else
        {
            /* No saved context. This should not ideally happen */
            return -1;
        }
    }

    return return_value;
}

/* Must only be used from environment call context */
void kernel_self_abort_save_context(uint64_t stack_frame)
{
    /* Get the kernel exception buffer */
    uint64_t exception_buffer = kernel_info_get_exception_buffer(get_shire_id());

    /* If the kernel exception buffer is available */
    if (exception_buffer != 0)
    {
        internal_execution_context_t context;

        /* Dump S-mode CSRs */
        CSR_READ_SCAUSE(context.scause)
        CSR_READ_SSTATUS(context.sstatus)
        CSR_READ_SEPC(context.sepc)
        CSR_READ_STVAL(context.stval)

        context.regs = (uint64_t *)stack_frame;

        /* Save the execution context in the buffer */
        CM_To_MM_Save_Execution_Context((execution_context_t *)exception_buffer,
            CM_CONTEXT_TYPE_SELF_ABORT, get_hart_id(), &context);
    }
}
