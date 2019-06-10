#include "kernel.h"
#include "layout.h"
#include "macros.h"
#include "message.h"
#include "shire.h"
#include "sync.h"
#include "syscall.h"
#include "test_kernels.h"

#include <stdbool.h>
#include <inttypes.h>

//#define USE_TEST_KERNEL

static const uint8_t tensorZeros[64] __attribute__ ((aligned (64))) = {0};

static void pre_kernel_setup(void);
static void kernel_return_function(void) __attribute__ ((used));
static void post_kernel_cleanup(void);

// Saves firmware context and launches kernel in user mode with clean stack and registers
int64_t launch_kernel(const uint64_t* const kernel_entry_addr,
                      const uint64_t* const kernel_stack_addr,
                      const uint64_t* const argument_ptr,
                      const uint64_t* const grid_ptr)
{
    uint64_t* firmware_sp;

    asm volatile (
        "csrr  %0, sscratch \n"
        "addi  %0, %0, 8    \n"
        : "=r" (firmware_sp)
    );

    pre_kernel_setup();

    // -Save firmware context on supervisor stack and sp to supervisor stack SP region
    // -Switch sp to kernel_stack_addr
    // -Setup ra and user stack frame so the kernel can ret to kernel_return_function
    // -Wipe register state (no leakage from S to U mode)
    // -sret to kernel_entry_addr in user mode
    asm volatile (
        "addi  sp, sp, -( 29 * 8 ) \n" // save context on stack (except ra, which is in clobber list)
        "la    ra,  1f             \n" // set return address to instruction after sret
        "sd    x1,  0  * 8( sp )   \n"
        "sd    x3,  1  * 8( sp )   \n"
        "sd    x5,  2  * 8( sp )   \n"
        "sd    x6,  3  * 8( sp )   \n"
        "sd    x7,  4  * 8( sp )   \n"
        "sd    x8,  5  * 8( sp )   \n"
        "sd    x9,  6  * 8( sp )   \n"
        "sd    x10, 7  * 8( sp )   \n"
        "sd    x11, 8  * 8( sp )   \n"
        "sd    x12, 9  * 8( sp )   \n"
        "sd    x13, 10 * 8( sp )   \n"
        "sd    x14, 11 * 8( sp )   \n"
        "sd    x15, 12 * 8( sp )   \n"
        "sd    x16, 13 * 8( sp )   \n"
        "sd    x17, 14 * 8( sp )   \n"
        "sd    x18, 15 * 8( sp )   \n"
        "sd    x19, 16 * 8( sp )   \n"
        "sd    x20, 17 * 8( sp )   \n"
        "sd    x21, 18 * 8( sp )   \n"
        "sd    x22, 19 * 8( sp )   \n"
        "sd    x23, 20 * 8( sp )   \n"
        "sd    x24, 21 * 8( sp )   \n"
        "sd    x25, 22 * 8( sp )   \n"
        "sd    x26, 23 * 8( sp )   \n"
        "sd    x27, 24 * 8( sp )   \n"
        "sd    x28, 25 * 8( sp )   \n"
        "sd    x29, 26 * 8( sp )   \n"
        "sd    x30, 27 * 8( sp )   \n"
        "sd    x31, 28 * 8( sp )   \n"
        "sd    sp, %0              \n" // save sp to supervisor stack SP region
        "mv    ra, %1              \n" // set return address to kernel_return_function
        "mv    s0, %2              \n" // switch to kernel stack: set s0 (frame pointer) to kernel_stack_addr
        "addi  sp, s0, -32         \n" // switch to kernel stack: set sp to kernel stack after stack frame
        "sd    ra, 24(sp)          \n" // push ra
        "sd    s0, 16(sp)          \n" // push s0
        "li    x5, 0x100           \n" // bitmask to clear sstatus SPP=user
        "csrc  sstatus, x5         \n" // clear sstatus SPP
        "csrsi sstatus, 0x10       \n" // set sstatus UPIE
        "csrw  sepc, %3            \n" // kernel address to jump to in user mode
        "mv    x3, zero            \n" // kernel must set its own gp if it uses it
        "mv    x4, zero            \n" // Wipe registers: don't leak state from S to U
        "mv    x5, zero            \n"
        "mv    x6, zero            \n"
        "mv    x7, zero            \n"
        "mv    x8, zero            \n"
        "mv    x9, zero            \n"
        "mv    x10, %4             \n" // a0 = argument_ptr
        "mv    x11, %5             \n" // a1 = grid_ptr
        "mv    x12, zero           \n"
        "mv    x13, zero           \n"
        "mv    x14, zero           \n"
        "mv    x15, zero           \n"
        "mv    x16, zero           \n"
        "mv    x17, zero           \n"
        "mv    x18, zero           \n"
        "mv    x19, zero           \n"
        "mv    x20, zero           \n"
        "mv    x21, zero           \n"
        "mv    x22, zero           \n"
        "mv    x23, zero           \n"
        "mv    x24, zero           \n"
        "mv    x25, zero           \n"
        "mv    x26, zero           \n"
        "mv    x27, zero           \n"
        "mv    x28, zero           \n"
        "mv    x29, zero           \n"
        "mv    x30, zero           \n"
        "mv    x31, zero           \n"
#ifdef __riscv_flen
        "fcvt.s.w f0,  x0          \n"
        "fcvt.s.w f1,  x0          \n"
        "fcvt.s.w f2,  x0          \n"
        "fcvt.s.w f3,  x0          \n"
        "fcvt.s.w f4,  x0          \n"
        "fcvt.s.w f5,  x0          \n"
        "fcvt.s.w f6,  x0          \n"
        "fcvt.s.w f7,  x0          \n"
        "fcvt.s.w f8,  x0          \n"
        "fcvt.s.w f9,  x0          \n"
        "fcvt.s.w f10, x0          \n"
        "fcvt.s.w f11, x0          \n"
        "fcvt.s.w f12, x0          \n"
        "fcvt.s.w f13, x0          \n"
        "fcvt.s.w f14, x0          \n"
        "fcvt.s.w f15, x0          \n"
        "fcvt.s.w f16, x0          \n"
        "fcvt.s.w f17, x0          \n"
        "fcvt.s.w f18, x0          \n"
        "fcvt.s.w f19, x0          \n"
        "fcvt.s.w f20, x0          \n"
        "fcvt.s.w f21, x0          \n"
        "fcvt.s.w f22, x0          \n"
        "fcvt.s.w f23, x0          \n"
        "fcvt.s.w f24, x0          \n"
        "fcvt.s.w f25, x0          \n"
        "fcvt.s.w f26, x0          \n"
        "fcvt.s.w f27, x0          \n"
        "fcvt.s.w f28, x0          \n"
        "fcvt.s.w f29, x0          \n"
        "fcvt.s.w f30, x0          \n"
        "fcvt.s.w f31, x0          \n"
#endif
        "sret                      \n"
        "1:                        \n"
        : "=m" (*firmware_sp)
        : "r" (kernel_return_function), "r" (kernel_stack_addr), "r" (kernel_entry_addr), "r" (argument_ptr), "r" (grid_ptr)
        : "ra" // SYSCALL_RETURN_FROM_KERNEL rets back to 1: so ra is clobbered. Rest of context is preserved.
    );

    post_kernel_cleanup();

    // TODO FIXME return kernel's return value
    return 0;
}

void kernel_function(void)
{
#ifdef USE_TEST_KERNEL
    // Test kernel:
    // // thread 0s run compute kernel
    if (get_thread_id() == 0)
    {
        // TODO get PC of compute kernel from master net_desc.compute_pc - IPI? Need to define this aspect of Minion API.
        test_compute_kernel();
    }
    // For now, all active thread 1s run prefetch kernel
    else if (get_thread_id() == 1)
    {
        // TODO get PC of prefetch kernel from master - IPI? net_desc only has compute_pc for now.
        test_prefetch_kernel();
    }
    else
    {
        asm volatile ("wfi");
    }
#endif
}

// Restores firmware context
int64_t return_from_kernel(void)
{
    uint64_t* firmware_sp;

    asm volatile (
        "csrr  %0, sscratch \n"
        "addi  %0, %0, 8    \n"
        : "=r" (firmware_sp)
    );

    if (*firmware_sp != 0)
    {
        // Switch to firmware stack
        // restore context from stack
        asm volatile (
            "li    x1, 0x120         \n" // bitmask to set sstatus SPP and SPIE
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
            "ld    x10, 7  * 8( sp ) \n"
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
            : "+m" (*firmware_sp)
        );

        return 0;
    }
    else
    {
        return -1;
    }
}

static void pre_kernel_setup(void)
{
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
    // "Attempting to write a value lower than 2 sets LogMsgSize to the value 2."
    asm volatile (
        "csrwi portctrl0, 0 \n"
        "csrwi portctrl1, 0 \n"
        "csrwi portctrl2, 0 \n"
        "csrwi portctrl3, 0 \n"
    );

    // Thread 0 in each minion
    if (get_thread_id() == 0)
    {
        uint64_t temp, temp2;

        // Zero out TensorExtensionCSRs
        asm volatile (
            "csrwi tensor_mask,  0 \n"
            "csrwi tensor_error, 0 \n"
            "csrwi tensor_coop,  0 \n"
        );

        // Zero out TenC
        asm volatile (
            "la    %0, tensorZeros        \n"
            "li    x31, 0                 \n" // 0-byte stride, ID 0
            "li    %1, 0x000000000000000F \n" // Load 16 lines to L1SP lines 0-15
            "or    %1, %0, %1             \n" // Set address of tensorLoad to tensorZeros
            "csrw  tensor_load, %1        \n"
            "csrwi tensor_wait, 0         \n"
            "li    %1, 0x020000000000000F \n" // Load 16 lines to L1SP lines 16-31
            "or    %1, %0, %1             \n" // Set address of tensorLoad to tensorZeros
            "csrw  tensor_load, %1        \n"
            "csrwi tensor_wait, 0         \n"
            "li    %1, 0x01FF800000610007 \n" // 16x16 TenC = 16x64 A in L1 SP lines 0-15 * 64x16 B in L1SP lines 16-31
            "csrw  tensor_fma, %1         \n"
            "csrwi tensor_wait, 7         \n"
            : "=&r" (temp), "=&r" (temp2)
            : "m" (tensorZeros[64])
            : "x31"
        );

        // Set rounding mode to 0 (round near even)
        asm volatile (
            "csrw frm, x0"
        );
    }
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
        // Last thread to join barrier sends FCC0 to all HARTs in this shire
        SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, 0xFFFFFFFFU);
        SEND_FCC(THIS_SHIRE, THREAD_1, FCC_0, 0xFFFFFFFFU);
    }

    // Wait until all HARTs are synchronized in post_kernel_cleanup before
    // draining coalescing buffer and evicting L1->L2 and L2->L3
    WAIT_FCC(0);

    syscall(SYSCALL_POST_KERNEL_CLEANUP, 0, 0, 0);

    WAIT_FLB(64, 31, result);

    // Last thread to join barrier
    if (result)
    {
        static message_t message = {.id = MESSAGE_ID_KERNEL_COMPLETE, .data = {0}};
        const uint64_t shire_id = get_shire_id();
        const uint64_t hart_id = get_hart_id();

        message.data[0] = shire_id;
        message.data[1] = hart_id;

        // Send message to master indicating the kernel is complete in this shire
        message_send_worker(shire_id, hart_id, &message);
    }
}
