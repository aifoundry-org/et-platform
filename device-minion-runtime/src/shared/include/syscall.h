#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

typedef enum {
    // from supervisor to machine for master firmware [0-99]
    SYSCALL_BROADCAST = 0,        // FW_MASTER_MCODE_ECALL_BROADCAST
    SYSCALL_IPI_TRIGGER = 1,      // FW_MASTER_MCODE_ECALL_IPI_TRIGGER
    SYSCALL_ENABLE_THREAD1 = 2,
    SYSCALL_PRE_KERNEL_SETUP = 3, // FW_MASTER_MCODE_ECALL_SHIRE_INIT
    SYSCALL_POST_KERNEL_CLEANUP = 4,
    SYSCALL_INIT_L1 = 6,
    SYSCALL_GET_MTIME = 7,

    // from supervisor to machine for compute firmware [200-299]
    SYSCALL_DRAIN_COALESCING_BUFFER = 200, // FW_COMPUTE_MCODE_ECALL_CB_DRAIN
    SYSCALL_EVICT_L1 = 201,      // FW_COMPUTE_MCODE_ECALL_L1_EVICT_ALL
    SYSCALL_UNLOCK_L1 = 202,     // FW_COMPUTE_MCODE_ECALL_L1_UNLOCK_ALL
    SYSCALL_EVICT_L2 = 203,      // FW_COMPUTE_MCODE_ECALL_L2_EVICT_ALL_START
    SYSCALL_EVICT_L2_WAIT = 204, // FW_COMPUTE_MCODE_ECALL_L2_EVICT_ALL_WAIT
    SYSCALL_CACHE_CONTROL = 205, // FW_COMPUTE_MCODE_ECALL_CACHE_CONTROL
    SYSCALL_FLUSH_L3 = 206,      // FW_COMPUTE_MCODE_ECALL_L3_FLUSH_ALL

    // from userspace to supervisor for compute firmware [300-399]
    //SYSCALL_KERNEL_CLEAR = 300,              // FW_COMPUTE_SCODE_ECALL_KERNEL_CLEAR
    SYSCALL_DRAIN_COALESCING_BUFFER_ALT = 301, // FW_COMPUTE_SCODE_ECALL_CB_DRAIN
    SYSCALL_CACHE_CONTROL_ALT = 302,           // FW_COMPUTE_SCODE_ECALL_CACHE_CONTROL
    SYSCALL_FLUSH_L3_ALT = 303,                // FW_COMPUTE_SCODE_ECALL_L3_FLUSH_ALL
    SYSCALL_RETURN_FROM_KERNEL = 304,

    // from userspace to supervisor for both master and compute firmwares [400-499]
    SYSCALL_LOG_WRITE = 400     // FW_SCODE_ECALL_LOG_WRITE
} syscall_t;


static inline __attribute__((always_inline)) int64_t syscall(syscall_t syscall, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    register  int64_t a0 asm("a0") = syscall;
    register uint64_t a1 asm("a1") = arg1;
    register uint64_t a2 asm("a2") = arg2;
    register uint64_t a3 asm("a3") = arg3;

    asm volatile (
        "ecall"
        : "+r" (a0)
        : "r" (a1), "r" (a2), "r" (a3)
        : "memory"
    );

    return a0;
}

#endif // SYSCALL_H
