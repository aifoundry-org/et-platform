#include "worker.h"
#include "layout.h"
#include "macros.h"
#include "shire.h"
#include "sync.h"
#include "syscall.h"
#include "test_kernels.h"

#include <stdbool.h>
#include <stdint.h>

#define MASTER_SHIRE 32U

uint64_t fw_sp[2048];

static void pre_kernel_setup(void) __attribute__ ((used));
static int launch_kernel(const uint64_t* const kernel_entry_addr, const uint64_t* const kernel_stack_addr);
static void kernel_function(void) __attribute__ ((used));
static void kernel_return_function(void) __attribute__ ((used));
static void post_kernel_cleanup(void) __attribute__ ((used));

void WORKER_thread(void)
{
    const uint64_t hart_id = get_hart_id();

    while (1)
    {
        pre_kernel_setup();

        // Wait for FCC1 from master indicating the kernel should start
        WAIT_FCC(1);

        // TODO FIXME right now all threads are waiting and synchronizing in setup/cleanup, but what if we don't want all?
        // Do we need to track which N are active per shire instead of assuming 64?

        const uint64_t* const kernel_entry_addr = (uint64_t*)kernel_function; // TODO FIXME parameter
        const uint64_t* const kernel_stack_addr = (uint64_t*)(KERNEL_STACK_BASE - (hart_id * KERNEL_STACK_SIZE)); // 4K per kernel TODO FIXME parameter

        // TODO FIXME be clever with kernel stack addresses to keep stacks on local memshires

        launch_kernel(kernel_entry_addr, kernel_stack_addr);

        post_kernel_cleanup();
    }
}

// Runs in S-mode, only need to syscall for M-mode services
static void pre_kernel_setup(void)
{
    bool result;

    // arg1 = 0 to enable all thread 1s
    syscall(SYSCALL_PRE_KERNEL_SETUP, 0, 0, 0);

    // First HART in the shire
    if (get_hart_id() % 64U == 0U)
    {
        // Init all kernel FLBs except reserved FLB 31
        for (unsigned int barrier = 0; barrier < 31; barrier++)
        {
            INIT_FLB(THIS_SHIRE, barrier);
        }
    }

    // Empty all FCCs
    for (uint64_t i = read_fcc(0); i > 0; i--)
    {
        WAIT_FCC(0);
    }

    for (uint64_t i = read_fcc(1); i > 0; i--)
    {
        WAIT_FCC(1);
    }

    // Disable message ports
    {
        // portctrl0(0);
        // portctrl1(0);
        // portctrl2(0);
        // portctrl3(0);
    }

    // Zero out TenC
    {
        // TODO (perform a 0*0 operation)
    }

    // TensorExtensionCSRs all 0
    {
        // TODO
        // tensormask_write(0);
        // tensorerror_write(0);
        // for (tmp = 0; tmp <= 255; tmp++)
        //     tensorcooperation_write(tmp);

    }

    // GPR and VPU set to 0s
    {
        // TODO
    }

    WAIT_FLB(64, 31, result);

    if (result)
    {
        // send FCC1 to master to let it know this shire is ready
        SEND_FCC(MASTER_SHIRE, THREAD_0, FCC_1, 1);
    }
}

// Saves firmware context and launches kernel in user mode with clean stack and registers
static int launch_kernel(const uint64_t* const kernel_entry_addr, const uint64_t* const kernel_stack_addr)
{
    // Test kernel:
    // // thread 0s run compute kernel
    // if (get_thread_id() == 0)
    // {
    //     // TODO get PC of compute kernel from master net_desc.compute_pc - IPI? Need to define this aspect of Minion API.
    //     test_compute_kernel();
    // }
    // // For now, all active thread 1s run prefetch kernel
    // else if (get_thread_id() == 1)
    // {
    //     // TODO get PC of prefetch kernel from master - IPI? net_desc only has compute_pc for now.
    //     test_prefetch_kernel();
    // }
    // else
    // {
    //     asm volatile ("wfi");
    // }

    const uint64_t hart_id = get_hart_id();

    if (fw_sp[hart_id] == 0)
    {
        // -Save context on supervisor stack and update fw_sp to point to saved context:
        //   SYSCALL_RETURN_FROM_KERNEL will save user context on the supervisor stack,
        //   we don't want to restore from that.
        // -Switch sp to kernel_stack_addr
        // -Store a frame on the kernel stack so it can ret to return_from_kernel_addr
        // -Wipe register state (no leakage from S to U mode)
        // -sret to kernel_entry_addr in user mode
        asm volatile (
            "addi  sp, sp, -( 29 * 8 )  \n" // save context on stack
            "sd    x1,  1  * 8( sp )    \n"
            "sd    x3,  2  * 8( sp )    \n"
            "sd    x5,  3  * 8( sp )    \n"
            "sd    x6,  4  * 8( sp )    \n"
            "sd    x7,  5  * 8( sp )    \n"
            "sd    x8,  6  * 8( sp )    \n"
            "sd    x9,  7  * 8( sp )    \n"
            "sd    x10, 8  * 8( sp )    \n"
            "sd    x11, 9  * 8( sp )    \n"
            "sd    x12, 10 * 8( sp )    \n"
            "sd    x13, 11 * 8( sp )    \n"
            "sd    x14, 12 * 8( sp )    \n"
            "sd    x15, 13 * 8( sp )    \n"
            "sd    x16, 14 * 8( sp )    \n"
            "sd    x17, 15 * 8( sp )    \n"
            "sd    x18, 16 * 8( sp )    \n"
            "sd    x19, 17 * 8( sp )    \n"
            "sd    x20, 18 * 8( sp )    \n"
            "sd    x21, 19 * 8( sp )    \n"
            "sd    x22, 20 * 8( sp )    \n"
            "sd    x23, 21 * 8( sp )    \n"
            "sd    x24, 22 * 8( sp )    \n"
            "sd    x25, 23 * 8( sp )    \n"
            "sd    x26, 24 * 8( sp )    \n"
            "sd    x27, 25 * 8( sp )    \n"
            "sd    x28, 26 * 8( sp )    \n"
            "sd    x29, 27 * 8( sp )    \n"
            "sd    x30, 28 * 8( sp )    \n"
            "sd    x31, 29 * 8( sp )    \n"
            "la    x1, 1f               \n" // load address of instruction after sret
            "addi  sp, sp, -8           \n"
            "sd    x1,   1 * 8( sp )    \n" // push address
            "sd    sp, %0               \n" // save sp to fw_sp[hart_id]
            "csrw  sscratch, sp         \n" // save sp to sscratch so subsequent S-mode traps use correct SP
            "mv    ra, %1               \n" // set ra to kernel_return_function
            "mv    s0, %2               \n" // switch to kernel stack: set s0 (frame pointer) to kernel_stack_addr
            "addi  sp, s0, -32          \n" // switch to kernel stack: set sp to kernel stack after stack frame
            "sd    ra, 24(sp)           \n" // push ra
            "sd    s0, 16(sp)           \n" // push s0
            "li    x5, 0x100            \n" // bitmask to clear sstatus SPP=user
            "csrc  sstatus, x5          \n" // clear sstatus SPP
            "csrsi sstatus, 0x10        \n" // set sstatus UPIE
            "csrw  sepc, %3             \n" // kernel address to jump to in user mode
            "mv    x3, x0               \n" // kernel_entry_addr must set its own gp
            "mv    x4, x0               \n" // Wipe registers: don't leak state from S to U
            "mv    x5, x0               \n"
            "mv    x6, x0               \n"
            "mv    x7, x0               \n"
            "mv    x8, x0               \n"
            "mv    x9, x0               \n"
            "mv    x10, x0              \n" // TODO pass arguments in a0, etc.
            "mv    x11, x0              \n"
            "mv    x12, x0              \n"
            "mv    x13, x0              \n"
            "mv    x14, x0              \n"
            "mv    x15, x0              \n"
            "mv    x16, x0              \n"
            "mv    x17, x0              \n"
            "mv    x18, x0              \n"
            "mv    x19, x0              \n"
            "mv    x20, x0              \n"
            "mv    x21, x0              \n"
            "mv    x22, x0              \n"
            "mv    x23, x0              \n"
            "mv    x24, x0              \n"
            "mv    x25, x0              \n"
            "mv    x26, x0              \n"
            "mv    x27, x0              \n"
            "mv    x28, x0              \n"
            "mv    x29, x0              \n"
            "mv    x30, x0              \n"
            "mv    x31, x0              \n"
            "sret                       \n"
            "1:                         \n"
            : "=m" (fw_sp[hart_id])
            : "r" (kernel_return_function), "r" (kernel_stack_addr), "r" (kernel_entry_addr)
            : "ra" // RETURN_FROM_KERNEL syscall rets back to 1: so ra is clobbered. Rest of context is preserved.
        );
    }

    return -1;
}

static void kernel_function(void)
{

}

// This must to a a RX function in user space, no W from user!
static void kernel_return_function(void)
{
    syscall(SYSCALL_RETURN_FROM_KERNEL, 0, 0, 0);
}

static void post_kernel_cleanup(void)
{
    bool result;

    // Wait for all tensor ops to complete
    WAIT_TENSOR_LOAD_0
    WAIT_TENSOR_LOAD_1
    WAIT_TENSOR_LOAD_L2_0
    WAIT_TENSOR_LOAD_L2_1
    WAIT_PREFETCH_0
    WAIT_PREFETCH_1
    WAIT_CACHEOPS
    WAIT_TENSOR_FMA
    WAIT_TENSOR_STORE
    WAIT_TENSOR_REDUCE

    // Empty FCC0
    for (uint64_t i = read_fcc(0); i > 0; i--)
    {
        WAIT_FCC(0);
    }

    WAIT_FLB(64, 31, result);

    if (result)
    {
        // send FCC0 to all HARTs in this shire
        SEND_FCC(0xFF, THREAD_0, FCC_0, 0xFFFFFFFFU);
        SEND_FCC(0xFF, THREAD_1, FCC_0, 0xFFFFFFFFU);
    }

    // Wait until all HARTs are synchronized in post_kernel_cleanup before
    // draining coalescing buffer and evicting L1->L2 and L2->L3
    WAIT_FCC(0);

    syscall(SYSCALL_POST_KERNEL_CLEANUP, 0, 0, 0);

    WAIT_FLB(64, 31, result);

    // Last thread to join barrier
    if (result)
    {
        // send FCC1 to master indicating the kernel is complete
        SEND_FCC(MASTER_SHIRE, THREAD_0, FCC_1, 1);
    }
}
