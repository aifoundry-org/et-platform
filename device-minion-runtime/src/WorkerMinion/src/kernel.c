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
    uint64_t completed_threads; /* Bitmask of threads that have already completed the launch */
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

static inline void kernel_info_set_execution_status(uint32_t shire_id, kernel_complete_status_e status)
{
    atomic_store_signed_local_8(&kernel_launch_info[shire_id].execution_status, status);
}

static inline kernel_complete_status_e kernel_info_get_execution_status(uint32_t shire_id)
{
    return atomic_load_signed_local_8(&kernel_launch_info[shire_id].execution_status);
}

static inline void kernel_info_reset_completed_threads(uint32_t shire_id)
{
    atomic_store_local_64(&kernel_launch_info[shire_id].completed_threads, 0);
}

// Returns previous completed mask
static inline uint64_t kernel_info_set_thread_completed(uint32_t shire_id, uint64_t thread_id)
{
    return atomic_or_local_64(&kernel_launch_info[shire_id].completed_threads,
        1ull << thread_id);
}

bool kernel_info_has_thread_completed(uint32_t shire_id, uint64_t thread_id)
{
    return (atomic_load_local_64(&kernel_launch_info[shire_id].completed_threads) >> thread_id) & 1;
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

void __attribute__((noreturn)) launch_kernel(uint8_t kw_base_id,
                      uint8_t slot_index,
                      uint64_t kernel_entry_addr,
                      uint64_t kernel_stack_addr,
                      uint64_t kernel_params_ptr,
                      uint64_t kernel_launch_flags,
                      uint64_t kernel_shire_mask)
{
    pre_kernel_setup(kw_base_id, slot_index, kernel_launch_flags);

    /* Wait until all the threads involved in the kernel launch are here */
    spinlock_barrier_global(&pre_launch_global_barrier, (uint32_t)__builtin_popcountll(kernel_shire_mask));

    // -Switch sp to kernel_stack_addr
    // -Set kernel arguments
    // -Wipe register state (no leakage from S to U mode)
    // -sret to kernel_entry_addr in user mode
    asm volatile(
        "mv    sp,  %[k_stack_addr]\n" // set stack pointer to kernel_stack_addr
        "mv    x10, %[k_param_a0]  \n" // a0 = kernel_params_ptr
        "mv    x11, %[k_param_a1]  \n" // a1 = UNUSED
        "mv    x12, %[k_param_a2]  \n" // a2 = UNUSED
        "mv    x13, %[k_param_a3]  \n" // a3 = UNUSED
        "mv    x1,  %[k_ret_addr]  \n" // set return address to 0 to catch kernels that don't end properly
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
        :
        : [k_ret_addr]    "r"(0),
          [k_stack_addr]  "r"(kernel_stack_addr),
          [k_entry]       "r"(kernel_entry_addr),
          [k_param_a0]    "r"(kernel_params_ptr),
          [k_param_a1]    "r"(0), /* Unused for now */
          [k_param_a2]    "r"(0), /* Unused for now */
          [k_param_a3]    "r"(0)  /* Unused for now */
    );

    /* Should never get here! */
    while (1)
    {

    }
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

        // Initialize the kernel execution status
        kernel_info_set_attributes(shire_id, kw_base_id, slot_index);
        kernel_info_set_execution_status(shire_id, KERNEL_COMPLETE_STATUS_SUCCESS);
        kernel_info_reset_completed_threads(shire_id);

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

void kernel_launch_post_cleanup(uint8_t kw_base_id, uint8_t slot_index, int64_t kernel_ret_val)
{
    const uint32_t shire_id = get_shire_id();
    const uint64_t thread_id = get_hart_id() & 63;
    const uint32_t thread_count = (get_shire_id() == MASTER_SHIRE) ? 32 : 64;
    const uint32_t minion_mask = (get_shire_id() == MASTER_SHIRE) ? 0xFFFF0000U : 0xFFFFFFFFU;
    const uint64_t thread_mask = (get_shire_id() == MASTER_SHIRE) ? 0xFFFFFFFF00000000U : 0xFFFFFFFFFFFFFFFFU;
    uint64_t prev_completed;

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

    /* Check for kernel execution error. Has to be done before the barrier */
    if (kernel_ret_val < KERNEL_COMPLETE_STATUS_SUCCESS)
    {
        // TODO: Use atomic OR/cmpxchg?
        kernel_info_set_execution_status(shire_id, KERNEL_COMPLETE_STATUS_ERROR);
    }

    // Blocking barrier with all the participating threads of the shire
    // We have to make sure all threads have finished before evicting caches
    local_fcc_barrier(&post_launch_barrier[shire_id], thread_count, minion_mask);

    syscall(SYSCALL_POST_KERNEL_CLEANUP_INT, thread_count, 0, 0);

    // Last thread to reach here sends done message to Kernel Worker thread of Master shire
    prev_completed = kernel_info_set_thread_completed(shire_id, thread_id);
    if ((prev_completed | (1ull << thread_id)) == thread_mask) {
        cm_to_mm_message_kernel_launch_completed_t msg;
        msg.header.number = 0; // Not used. TODO: Remove
        msg.header.id = CM_TO_MM_MESSAGE_ID_KERNEL_COMPLETE;
        msg.shire_id = shire_id;
        msg.slot_index = slot_index;
        msg.status = kernel_info_get_execution_status(shire_id);
        CM_To_MM_Iface_Unicast_Send((uint64_t)(kw_base_id + slot_index),
            (uint64_t)(1 + slot_index), (cm_iface_message_t *)&msg);
    }
}
