#include "device-common/hart.h"
#include "kernel.h"
#include "kernel_return.h"
#include "kernel_error.h"
#include "log.h"
#include "mm_to_cm_iface.h"
#include <inttypes.h>

/* Restores firmware context and resumes execution in launch_kernel()
Called from machine context by M-mode trap handler (e.g. if the kernel takes an exception)
and by firmware (e.g. normal kernel completion, kernel abort) */
int64_t return_from_kernel(int64_t return_value)
{
    const uint64_t thread_id = get_hart_id() & 63;
    uint64_t prev_returned = kernel_info_set_thread_returned( get_shire_id(), thread_id);

    /* A Hart in specific kernel launch can only pass through return_from_kernel once */
    if(!((prev_returned >> thread_id) & 1))
    {
        uint64_t *firmware_sp;

        asm volatile("csrr  %0, sscratch \n"
                    "addi  %0, %0, 8    \n"
                    : "=r"(firmware_sp));

        if (*firmware_sp != 0)
        {
            // Switch to firmware stack
            // restore context from stack
            asm volatile("li    x1, 0x120         \n" // bitmask to set sstatus SPP and SPIE
                        "csrs  sstatus, x1       \n" // set sstatus SPP and SPIE
                        "ld    sp, %0            \n" // load sp from supervisor stack SP region
                        "sd    zero, %0          \n" // clear supervisor stack SP region
                        "ld    x1,  0  * 8( sp ) \n" // restore context
                        "ld    x3,  1  * 8( sp ) \n"
                        "ld    x5,  2  * 8( sp ) \n"
                        "ld    x6,  3  * 8( sp ) \n"
                        "ld    x7,  4  * 8( sp ) \n"
                        "ld    x8,  5  * 8( sp ) \n"
                        "ld    x9,  6  * 8( sp ) \n"
                        // Leave return_value from kernel in a0
                        "ld    x11, 8  * 8( sp ) \n"
                        "ld    x12, 9  * 8( sp ) \n"
                        "ld    x13, 10 * 8( sp ) \n"
                        "ld    x14, 11 * 8( sp ) \n"
                        "ld    x15, 12 * 8( sp ) \n"
                        "ld    x16, 13 * 8( sp ) \n"
                        "ld    x17, 14 * 8( sp ) \n"
                        "ld    x18, 15 * 8( sp ) \n"
                        "ld    x19, 16 * 8( sp ) \n"
                        "ld    x20, 17 * 8( sp ) \n"
                        "ld    x21, 18 * 8( sp ) \n"
                        "ld    x22, 19 * 8( sp ) \n"
                        "ld    x23, 20 * 8( sp ) \n"
                        "ld    x24, 21 * 8( sp ) \n"
                        "ld    x25, 22 * 8( sp ) \n"
                        "ld    x26, 23 * 8( sp ) \n"
                        "ld    x27, 24 * 8( sp ) \n"
                        "ld    x28, 25 * 8( sp ) \n"
                        "ld    x29, 26 * 8( sp ) \n"
                        "ld    x30, 27 * 8( sp ) \n"
                        "ld    x31, 28 * 8( sp ) \n"
                        "addi  sp, sp, 29 * 8    \n"
                        "ret                     \n"
                        : "+m"(*firmware_sp));

            return return_value;
        }
        else
        {
            return KERNEL_LAUNCH_ERROR_NO_SAVED_CONTEXT;
        }
    }

    return return_value;
}
