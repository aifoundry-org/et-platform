#include "cacheops.h"
#include "kernel.h"
#include "fcc.h"
#include "flb.h"
#include "hart.h"
#include "layout.h"
#include "log.h"
#include "macros.h"
#include "message_types.h"
#include "cm_to_mm_iface.h"
#include "printf.h"
#include "sync.h"
#include "syscall_internal.h"

#include <stdbool.h>
#include <inttypes.h>

// Align the struct to cache line so that we can use local atomics on the array created below
typedef struct kernel_launch_info {
    union {
        struct {
            uint8_t kw_base_id;
            uint8_t slot_index;
        };
        uint16_t raw_u16;
    };
    int8_t execution_status;
} __attribute__((aligned(CACHE_LINE_SIZE))) kernel_launch_info_t;

static const uint8_t tensor_zeros[64] __attribute__((aligned(64))) = { 0 };

static local_fcc_barrier_t post_launch_barrier[NUM_SHIRES] = { 0 };
static spinlock_t post_launch_thread_count[NUM_SHIRES]= { 0 };
static spinlock_t pre_launch_local_barrier[NUM_SHIRES] = { 0 };
static spinlock_t pre_launch_global_barrier = { 0 };
static kernel_launch_info_t kernel_launch_info[NUM_SHIRES] = { 0 };

// This is a temporary hack. Should be properly done.
static inline bool spinlock_barrier_local(spinlock_t *lock, uint32_t num_threads)
{
    if (atomic_add_local_32(&lock->flag, 1U) == (num_threads - 1)) {
        return true;
    } else {
        do {
            asm volatile("fence\n" ::: "memory");
        } while (atomic_load_local_32(&lock->flag) != num_threads);
        return false;
    }
}

// This is a temporary hack. Should be properly done.
static void spinlock_barrier_global(spinlock_t *lock, uint32_t num_shires)
{
    const uint64_t shire_id = get_shire_id();
    const uint32_t thread_count = (get_shire_id() == MASTER_SHIRE) ? 32 : 64;
    bool last;

    last = spinlock_barrier_local(&pre_launch_local_barrier[shire_id], thread_count);

    // One thread per shire increments
    if (last)
        atomic_add_global_32(&lock->flag, 1U);

    do {
        asm volatile("fence\n" ::: "memory");
    } while (atomic_load_global_32(&lock->flag) != num_shires);

    // Reset primitives
    if (last) {
        init_local_spinlock(&pre_launch_local_barrier[shire_id], 0);
        if (shire_id == 0)
            init_global_spinlock(&pre_launch_global_barrier, 0);
    }
}

void kernel_info_get_attributes(uint32_t shire_id, uint8_t *kw_base_id, uint8_t *slot_index)
{
    kernel_launch_info_t kernel_info;

    /* Load the kernel info */
    kernel_info.raw_u16 = atomic_load_local_16(&kernel_launch_info[shire_id].raw_u16);

    /* Return the required attributes */
    *kw_base_id = kernel_info.kw_base_id;
    *slot_index = kernel_info.slot_index;
}

static inline void kernel_info_set_attributes(uint32_t shire_id, uint8_t kw_base_id, uint8_t slot_index)
{
    kernel_launch_info_t kernel_info;

    /* Save the attributes */
    kernel_info.kw_base_id = kw_base_id;
    kernel_info.slot_index = slot_index;
    atomic_store_local_16(&kernel_launch_info[shire_id].raw_u16, kernel_info.raw_u16);
}

static void pre_kernel_setup(uint8_t kw_base_id, uint8_t slot_index, uint64_t kernel_launch_flags);
static void post_kernel_cleanup(uint8_t kw_base_id, uint8_t slot_index, uint64_t kernel_launch_flags, int64_t kernel_ret_val);

// Saves firmware context and launches kernel in user mode with clean stack and registers
// Note that global Supervisor interrupts are disabled after returning from this function
int64_t launch_kernel(uint8_t kw_base_id,
                      uint8_t slot_index,
                      uint64_t kernel_entry_addr,
                      uint64_t kernel_stack_addr,
                      uint64_t kernel_params_ptr,
                      uint64_t kernel_launch_flags,
                      uint64_t kernel_shire_mask)
{
    uint64_t *firmware_sp;
    int64_t return_value;
    uint64_t tensor_error;

    asm volatile("csrr  %0, sscratch \n"
                 "addi  %0, %0, 8    \n"
                 : "=r"(firmware_sp));

    pre_kernel_setup(kw_base_id, slot_index, kernel_launch_flags);

    /* Wait until all the threads involved in the kernel launch are here */
    spinlock_barrier_global(&pre_launch_global_barrier, (uint32_t)__builtin_popcountll(kernel_shire_mask));

    // -Save firmware context on supervisor stack and sp to supervisor stack SP region
    // -Switch sp to kernel_stack_addr
    // -Setup ra and user stack frame so the kernel can ret to kernel_return_function
    // -Wipe register state (no leakage from S to U mode)
    // -sret to kernel_entry_addr in user mode
    asm volatile(
        "addi  sp, sp, -( 29 * 8 ) \n" // save context on stack (except ra, which is in clobber list)
        "la    ra,  fw_resume      \n" // set return address to instruction after sret
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
        "mv    x10, %[k_param_a0]  \n" // a0 = kernel_params_ptr
        "mv    x11, %[k_param_a1]  \n" // a1 = UNUSED
        "mv    x12, %[k_param_a2]  \n" // a2 = UNUSED
        "mv    x13, %[k_param_a3]  \n" // a3 = UNUSED
        "sd    sp, %[firmware_sp]  \n" // save sp to supervisor stack SP region (sscratch + 8)
        "mv    ra, %[k_ret_addr]   \n" // set return address to 0 to catch kernels that don't end properly
        "mv    s0, %[k_stack_addr] \n" // switch to kernel stack: set s0 (frame pointer) to kernel_stack_addr
        "addi  sp, s0, -32         \n" // switch to kernel stack: set sp to kernel stack after stack frame
        "sd    ra, 24(sp)          \n" // push ra
        "sd    s0, 16(sp)          \n" // push s0
        "li    x5, 0x100           \n" // bitmask to clear sstatus SPP=user
        "csrc  sstatus, x5         \n" // clear sstatus SPP
        "csrsi sstatus, 0x10       \n" // set sstatus UPIE
        "csrw  sepc, %[k_entry]    \n" // kernel address to jump to in user mode
        "mv    x3, zero            \n" // kernel must set its own gp if it uses it
        "mv    x4, zero            \n" // Wipe registers: don't leak state from S to U
        "mv    x5, zero            \n"
        "mv    x6, zero            \n"
        "mv    x7, zero            \n"
        "mv    x8, zero            \n"
        "mv    x9, zero            \n"
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
        "fw_resume:                \n" // firmware context resumes from here via return_from_kernel()
        "mv    %[return_value], a0 \n" // collect kernel return value
        "csrr  %[tensor_error], tensor_error \n" // collect tensor_error
        : [firmware_sp]  "=m"(*firmware_sp),
          [return_value] "=r"(return_value),
          [tensor_error] "=r"(tensor_error)
        : [k_ret_addr]    "r"(0),
          [k_stack_addr]  "r"(kernel_stack_addr),
          [k_entry]       "r"(kernel_entry_addr),
          [k_param_a0]    "r"(kernel_params_ptr),
          [k_param_a1]    "r"(0), /* Unused for now */
          [k_param_a2]    "r"(0), /* Unused for now */
          [k_param_a3]    "r"(0)  /* Unused for now */
        : "ra" // SYSCALL_RETURN_FROM_KERNEL rets back to 1: so ra is clobbered. Rest of context is preserved.
    );

    /* TODO: save the return_value and tensor_error in kernel exception/error buffer (if available) */

    post_kernel_cleanup(kw_base_id, slot_index, kernel_launch_flags, return_value);

    return return_value;
}

static void pre_kernel_setup(uint8_t kw_base_id, uint8_t slot_index, uint64_t kernel_launch_flags)
{
    const uint32_t shire_id = get_shire_id();
    const uint32_t minion_mask = (shire_id == MASTER_SHIRE) ? 0xFFFF0000U : 0xFFFFFFFFU;
    const uint64_t first_worker = (shire_id == MASTER_SHIRE) ? 32 : 0;

    // Enable Thread 1, init L1, invalidate I-cache
    //   arg1 = enable all worker thread 1s of the shire
    //   arg2 = first worker hart of the shire
    syscall(SYSCALL_PRE_KERNEL_SETUP_INT, minion_mask, first_worker, 0);

    // Second worker HART (first minion thread 1) in the shire
    // Thread 0s have more init to do than thread 1s, so use a thread 1 for per-shire init
    if ((get_hart_id() % 64U) == (first_worker + 1)) {

        // Save kernel info for each shire
        kernel_info_set_attributes(shire_id, kw_base_id, slot_index);

        // Initialize the kernel execution status
        atomic_store_signed_local_8(&kernel_launch_info[shire_id].execution_status,
            KERNEL_COMPLETE_STATUS_SUCCESS);

        // Init all FLBs except reserved FLBs 28-31
        for (uint64_t barrier = 0; barrier < 28; barrier++) {
            INIT_FLB(THIS_SHIRE, barrier);
        }

        // Enable cooperative TensorLoads and TensorStores in this shire
        volatile uint64_t *const shire_coop_mode_ptr =
            (volatile uint64_t *)ESR_SHIRE(THIS_SHIRE, SHIRE_COOP_MODE);
        *shire_coop_mode_ptr = 1;

        // Init post-kernel launch barrier
        local_fcc_barrier_init(&post_launch_barrier[shire_id]);
    }

    // [SW-3260] Force L3 evict in the firmware before starting a kernel - for performance analysis
    if (kernel_launch_flags & KERNEL_LAUNCH_FLAGS_EVICT_L3_BEFORE_LAUNCH) {
        // First Thread of first Minion of Shires 0-31 evict their L3 chunk
        // NOTE: This will only evict the whole L3 if all the 32 Shires participate in the launch
        if ((get_hart_id() % 64U == 0) && (shire_id < 32))
            syscall(SYSCALL_EVICT_L3_INT, 0, 0, 0);
    }

    // Empty all FCCs
    init_fcc(FCC_0);
    init_fcc(FCC_1);

    // Disable message ports
    // "Attempting to write a value lower than 2 sets LogMsgSize to the value 2."
    asm volatile("csrwi portctrl0, 0 \n"
                 "csrwi portctrl1, 0 \n"
                 "csrwi portctrl2, 0 \n"
                 "csrwi portctrl3, 0 \n");

    // Thread 0 in each minion
    if (get_thread_id() == 0) {
        uint64_t temp, temp2;

        // Perform a dummy TensorFMA to consume an unpaired TensorLoadSetupB, if any (rare)
        // If there isn't an unpaired TensorLoadSetupB (common case), this just generates a TensorError
        asm volatile("li    %0, 0x0000000000100006 \n" // B in memory, TensorType = IMA8A32
                     "csrw  tensor_fma, %0         \n"
                     "csrwi tensor_wait, 7         \n"
                     : "=&r"(temp));

        // Zero out TensorExtensionCSRs - previous FMA typically causes a TensorError
        asm volatile("csrwi tensor_mask,  0 \n"
                     "csrwi tensor_error, 0 \n"
                     "csrwi tensor_coop,  0 \n");

        // Zero out TenC
        asm volatile(
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
            : "=&r"(temp), "=&r"(temp2)
            : "m"(tensor_zeros[64])
            : "x31");

        // Enables 8 lanes of FPU, clears m1-m7
        asm volatile("li       %0, 0xFF \n" // m0=0xFF, m1-m7=0
                     "mova.m.x %0       \n"
                     : "=&r"(temp));

        // Ensure all cache evicts are complete
        WAIT_CACHEOPS
    }

    // Clear floating point flags and set rounding mode to 0 (round near even)
    asm volatile("csrw fcsr, zero");

    // Ensure all FLB and FCC init is complete
    asm volatile("fence");
}

static void post_kernel_cleanup(uint8_t kw_base_id, uint8_t slot_index, uint64_t kernel_launch_flags, int64_t kernel_ret_val)
{
    const uint32_t shire_id = get_shire_id();
    const uint32_t thread_count = (get_shire_id() == MASTER_SHIRE) ? 32 : 64;
    const uint32_t minion_mask = (get_shire_id() == MASTER_SHIRE) ? 0xFFFF0000U : 0xFFFFFFFFU;
    uint64_t evict_l3 = 0;

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

    // Blocking barrier with all the participating threads of the shire
    // We have to make sure all threads have finished before evicting caches
    local_fcc_barrier(&post_launch_barrier[shire_id], thread_count, minion_mask);

    // Check if we should evict the L3 to DDR after the kernel launch
    if (kernel_launch_flags & KERNEL_LAUNCH_FLAGS_EVICT_L3_AFTER_LAUNCH)
        evict_l3 = 1;

    syscall(SYSCALL_POST_KERNEL_CLEANUP_INT, thread_count, evict_l3, 0);

    // Check for kernel execution error
    if (kernel_ret_val < KERNEL_COMPLETE_STATUS_SUCCESS)
    {
        atomic_store_signed_local_8(&kernel_launch_info[shire_id].execution_status,
            KERNEL_COMPLETE_STATUS_ERROR);
    }

    // Last thread to reach here sends done message to Kernel Worker thread of Master shire
    if (atomic_add_local_32(&post_launch_thread_count[shire_id].flag, 1) == (thread_count - 1)) {
        // Reset counter
        atomic_store_local_32(&post_launch_thread_count[shire_id].flag, 0);

        cm_to_mm_message_kernel_launch_completed_t msg;
        msg.header.number = 0; // Not used. TODO: Remove
        msg.header.id = CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE;
        msg.shire_id = shire_id;
        msg.slot_index = slot_index;
        msg.status = atomic_load_signed_local_8(&kernel_launch_info[shire_id].execution_status);
        CM_To_MM_Iface_Unicast_Send((uint64_t)(kw_base_id + slot_index),
            (uint64_t)(1 + slot_index), (cm_iface_message_t *)&msg);
    }
}
