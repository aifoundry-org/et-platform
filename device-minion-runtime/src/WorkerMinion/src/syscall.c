#include "syscall.h"
#include "cacheops.h"
#include "shire.h"

#include <stdint.h>

int64_t syscall_handler(syscall_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3);

static int64_t return_from_kernel(void);

int64_t syscall_handler(syscall_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    int64_t rv;

    switch (number)
    {
        case SYSCALL_EVICT_L1_TO_L2:
        case SYSCALL_EVICT_L2_TO_L3:
        case SYSCALL_INIT_L1:
        case SYSCALL_DRAIN_COALESCING_BUFFER:
        case SYSCALL_ENABLE_THREAD1:
        case SYSCALL_BROADCAST:
        case SYSCALL_IPI_TRIGGER:
            rv = syscall(number, arg1, arg2, arg3); // forward the syscall to the machine mode handler
        break;

        case SYSCALL_RETURN_FROM_KERNEL:
            rv = return_from_kernel();
        break;

        case SYSCALL_PRE_KERNEL_SETUP:
        case SYSCALL_POST_KERNEL_CLEANUP:
            rv = -1; // user master minion firmware should never ask the supervisor master minon firmware to do this
        break;


        default:
            rv = -1; // unhandled s
        break;
    }

    return rv;
}

// Restores firmware context from user mode after a kernel completes
static int64_t return_from_kernel(void)
{
    extern uint64_t fw_sp[2048];

    const uint64_t hart_id = get_hart_id();

    if (fw_sp[hart_id] != 0)
    {
        // Switch to kernel stack via fw_sp
        // restore context from stack
        asm volatile (
            "li   x1, 0x120            \n" // bitmask to set sstatus SPP and SPIE
            "csrs sstatus, x1          \n" // set sstatus SPP and SPIE
            "ld   sp,  %0              \n" // load sp from fw_sp[hart_id]
            "sd   x0,  %0              \n" // clear fw_sp[hart_id]
            "ld   x1,  1  * 8( sp )    \n" // load pc from stack
            "ld   x3,  3  * 8( sp )    \n" // restore context
            "ld   x5,  4  * 8( sp )    \n"
            "ld   x6,  5  * 8( sp )    \n"
            "ld   x7,  6  * 8( sp )    \n"
            "ld   x8,  7  * 8( sp )    \n"
            "ld   x9,  8  * 8( sp )    \n"
            "ld   x10, 9  * 8( sp )    \n"
            "ld   x11, 10 * 8( sp )    \n"
            "ld   x12, 11 * 8( sp )    \n"
            "ld   x13, 12 * 8( sp )    \n"
            "ld   x14, 13 * 8( sp )    \n"
            "ld   x15, 14 * 8( sp )    \n"
            "ld   x16, 15 * 8( sp )    \n"
            "ld   x17, 16 * 8( sp )    \n"
            "ld   x18, 17 * 8( sp )    \n"
            "ld   x19, 18 * 8( sp )    \n"
            "ld   x20, 19 * 8( sp )    \n"
            "ld   x21, 20 * 8( sp )    \n"
            "ld   x22, 21 * 8( sp )    \n"
            "ld   x23, 22 * 8( sp )    \n"
            "ld   x24, 23 * 8( sp )    \n"
            "ld   x25, 24 * 8( sp )    \n"
            "ld   x26, 25 * 8( sp )    \n"
            "ld   x27, 26 * 8( sp )    \n"
            "ld   x28, 27 * 8( sp )    \n"
            "ld   x29, 28 * 8( sp )    \n"
            "ld   x30, 29 * 8( sp )    \n"
            "ld   x31, 30 * 8( sp )    \n"
            "addi sp, sp, 30 * 8       \n"
            "ret                       \n"
            : "+m" (fw_sp[hart_id])
        );
    }

    return -1;
}
