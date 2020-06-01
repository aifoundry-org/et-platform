#include "kernel.h"
#include "kernel_sync.h"
#include "cacheops.h"
#include "kernel_info.h"
#include "fcc.h"
#include "flb.h"
#include "hart.h"
#include "layout.h"
#include "log.h"
#include "macros.h"
#include "message.h"
#include "printf.h"
#include "syscall_internal.h"

#include <stdbool.h>
#include <inttypes.h>

static const uint8_t tensor_zeros[64] __attribute__ ((aligned (64))) = {0};

static void pre_kernel_setup(const kernel_params_t* const kernel_params_ptr, const grid_config_t* const grid_config_ptr);
static void kernel_return_function(int64_t return_value) __attribute__ ((used, section(".user_text"))); // must be placed in U-mode accessible section
static void log_errors(int64_t return_value, uint64_t tensor_error);
static void post_kernel_cleanup(const kernel_params_t* const kernel_params_ptr);

// Saves firmware context and launches kernel in user mode with clean stack and registers
int64_t launch_kernel(const uint64_t* const kernel_entry_addr,
                      const uint64_t* const kernel_stack_addr,
                      const kernel_params_t* const kernel_params_ptr,
                      const grid_config_t* const grid_config_ptr)
{
    uint64_t* firmware_sp;
    int64_t return_value;
    uint64_t tensor_error;

    asm volatile (
        "csrr  %0, sscratch \n"
        "addi  %0, %0, 8    \n"
        : "=r" (firmware_sp)
    );

    pre_kernel_setup(kernel_params_ptr, grid_config_ptr);

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
        "mv    x10, %6             \n" // a0 = kernel_params_ptr
        "mv    x11, %7             \n" // a1 = grid_config_ptr
        "sd    sp, %0              \n" // save sp to supervisor stack SP region
        "mv    ra, %3              \n" // set return address to kernel_return_function
        "mv    s0, %4              \n" // switch to kernel stack: set s0 (frame pointer) to kernel_stack_addr
        "addi  sp, s0, -32         \n" // switch to kernel stack: set sp to kernel stack after stack frame
        "sd    ra, 24(sp)          \n" // push ra
        "sd    s0, 16(sp)          \n" // push s0
        "li    x5, 0x100           \n" // bitmask to clear sstatus SPP=user
        "csrc  sstatus, x5         \n" // clear sstatus SPP
        "csrsi sstatus, 0x10       \n" // set sstatus UPIE
        "csrw  sepc, %5            \n" // kernel address to jump to in user mode
        "mv    x3, zero            \n" // kernel must set its own gp if it uses it
        "mv    x4, zero            \n" // Wipe registers: don't leak state from S to U
        "mv    x5, zero            \n"
        "mv    x6, zero            \n"
        "mv    x7, zero            \n"
        "mv    x8, zero            \n"
        "mv    x9, zero            \n"
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
        "sret                      \n" // ret to kernel_entry_addr in user mode
        "1:                        \n" // firmware context resumes from here via return_from_kernel()
        "mv    %1, a0              \n" // collect kernel return value
        "csrr  %2, tensor_error    \n" // collect tensor_error
        : "=m" (*firmware_sp), "=r" (return_value), "=r" (tensor_error)
        : "r" (kernel_return_function), "r" (kernel_stack_addr), "r" (kernel_entry_addr), "r" (kernel_params_ptr), "r" (grid_config_ptr)
        : "ra" // SYSCALL_RETURN_FROM_KERNEL rets back to 1: so ra is clobbered. Rest of context is preserved.
    );

    log_errors(return_value, tensor_error);

    post_kernel_cleanup(kernel_params_ptr);

    return return_value;
}

static void pre_kernel_setup(const kernel_params_t* const kernel_params_ptr, __attribute__((unused)) const grid_config_t* const grid_config_ptr)
{
    const uint64_t shire_id = get_shire_id();
    const uint32_t minion_mask = (shire_id == MASTER_SHIRE) ? 0xFFFF0000U : 0xFFFFFFFFU;
    const uint32_t thread_count = (shire_id == MASTER_SHIRE) ? 32 : 64;
    const uint64_t first_worker = (shire_id == MASTER_SHIRE) ? 32 : 0;

    // arg1 = enable all worker thread 1s of the shire
    // arg2 = first worker hart of the shire
    syscall(SYSCALL_PRE_KERNEL_SETUP_INT, minion_mask, first_worker, 0);

    // Second worker HART (first minion thread 1) in the shire
    // Thread 0s have more init to do than thread 1s, so use a thread 1 for per-shire init
    if ((get_hart_id() % 64U) == (first_worker + 1))
    {
        // Init all FLBs except reserved FLBs 28-31
        for (uint64_t barrier = 0; barrier < 28; barrier++)
        {
            INIT_FLB(THIS_SHIRE, barrier);
        }

        // Enable cooperative TensorLoads and TensorStores in this shire
        volatile uint64_t* const shire_coop_mode_ptr = (volatile uint64_t *)ESR_SHIRE(THIS_SHIRE, SHIRE_COOP_MODE);
        *shire_coop_mode_ptr = 1;
    }

    // Empty all FCCs
    init_fcc(FCC_0);
    init_fcc(FCC_1);

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

        // Perform a dummy TensorFMA to consume an unpaired TensorLoadSetupB, if any (rare)
        // If there isn't an unpaired TensorLoadSetupB (common case), this just generates a TensorError
        asm volatile (
            "li    %0, 0x0000000000100006 \n" // B in memory, TensorType = IMA8A32
            "csrw  tensor_fma, %0         \n"
            "csrwi tensor_wait, 7         \n"
            : "=&r" (temp)
        );

        // Zero out TensorExtensionCSRs - previous FMA typically causes a TensorError
        asm volatile (
            "csrwi tensor_mask,  0 \n"
            "csrwi tensor_error, 0 \n"
            "csrwi tensor_coop,  0 \n"
        );

        // Zero out TenC
        asm volatile (
            "la    %0, tensor_zeros       \n"
            "li    x31, 0                 \n" // 0-byte stride, ID 0
            "li    %1, 0x000000000000000F \n" // Load 16 lines to L1SP lines 0-15
            "or    %1, %0, %1             \n" // Set address of tensorLoad to tensor_zeros
            "csrw  tensor_load, %1        \n"
            "csrwi tensor_wait, 0         \n"
            "li    %1, 0x020000000000000F \n" // Load 16 lines to L1SP lines 16-31
            "or    %1, %0, %1             \n" // Set address of tensorLoad to tensor_zeros
            "csrw  tensor_load, %1        \n"
            "csrwi tensor_wait, 0         \n"
            "li    %1, 0x01FF800000610007 \n" // 16x16 TenC = 16x64 A in L1 SP lines 0-15 * 64x16 B in L1SP lines 16-31
            "csrw  tensor_fma, %1         \n"
            "csrwi tensor_wait, 7         \n"
            : "=&r" (temp), "=&r" (temp2)
            : "m" (tensor_zeros[64])
            : "x31"
        );

        // Enables 8 lanes of FPU, clears m1-m7
        asm volatile (
            "li       %0, 0xFF \n" // m0=0xFF, m1-m7=0
            "mova.m.x %0       \n"
            : "=&r" (temp)
        );

        // Ensure all cache evicts are complete
        WAIT_CACHEOPS
    }

    // Clear floating point flags and set rounding mode to 0 (round near even)
    asm volatile ("csrw fcsr, zero");

    // Ensure all FLB and FCC init is complete
    asm volatile ("fence");

    bool result;

    WAIT_FLB(thread_count, 28, result);

    // Last thread to join barrier sends ready FCC1 to master shire sync thread
    if (result)
    {
        notify_kernel_sync_thread(kernel_params_ptr->kernel_id, FCC_1);
    }

    // Wait for go FCC1 from master shire sync thread
    WAIT_FCC(1);
}


static void kernel_return_function(int64_t return_value)
{
    syscall(SYSCALL_RETURN_FROM_KERNEL, (uint64_t)return_value, 0, 0);
}

static void log_errors(int64_t return_value, uint64_t tensor_error)
{
    if ((return_value < 0) && (return_value != KERNEL_LAUNCH_ERROR_ABORTED))
    {
        log_write(LOG_LEVEL_ERROR, "return_value %" PRId64, return_value);
    }

    if (tensor_error != 0)
    {
        log_write(LOG_LEVEL_ERROR, "tensor_error 0x%016" PRIx64, tensor_error);
    }
}

static void post_kernel_cleanup(const kernel_params_t* const kernel_params_ptr)
{
    bool result;
    const uint32_t thread_count = (get_shire_id() == MASTER_SHIRE) ? 32 : 64;
    const uint32_t minion_mask = (get_shire_id() == MASTER_SHIRE) ? 0xFFFF0000U : 0xFFFFFFFFU;

    // All accesses to kernel_params must happen before SYSCALL_POST_KERNEL_CLEANUP_INT
    // evicts all the caches to avoid pulling it back in as a valid line
    const uint64_t kernel_id = kernel_params_ptr->kernel_id;

    // Wait for all memory accesses to complete
    FENCE

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
    init_fcc(FCC_0);

    WAIT_FLB(thread_count, 29, result);

    if (result)
    {
        // Last thread to join barrier sends FCC0 to all HARTs in this shire
        SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, minion_mask);
        SEND_FCC(THIS_SHIRE, THREAD_1, FCC_0, minion_mask);
    }

    // Wait until all HARTs are synchronized in post_kernel_cleanup before
    // draining coalescing buffer and evicting L1->L2 and L2->L3
    WAIT_FCC(0);

    syscall(SYSCALL_POST_KERNEL_CLEANUP_INT, thread_count, 0, 0);

    WAIT_FLB(thread_count, 31, result);

    // Last thread to join barrier sends done FCC1 to master shire sync thread
    if (result)
    {
        notify_kernel_sync_thread(kernel_id, FCC_1);
    }
}
