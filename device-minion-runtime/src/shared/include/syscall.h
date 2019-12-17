#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

typedef enum {
    // from supervisor to machine for master firmware [0-99]
    SYSCALL_BROADCAST = 0,
    SYSCALL_IPI_TRIGGER = 1,
    SYSCALL_ENABLE_THREAD1 = 2,
    SYSCALL_PRE_KERNEL_SETUP = 3,
    SYSCALL_POST_KERNEL_CLEANUP = 4,
    SYSCALL_INIT_L1 = 6,
    SYSCALL_GET_MTIME = 7,

    // from supervisor to machine for compute firmware [200-299]
    SYSCALL_DRAIN_COALESCING_BUFFER = 200,
    SYSCALL_EVICT_L1 = 201,
    SYSCALL_UNLOCK_L1 = 202,
    SYSCALL_EVICT_L2 = 203,
    SYSCALL_EVICT_L2_WAIT = 204,
    SYSCALL_CACHE_CONTROL = 205,
    SYSCALL_FLUSH_L3 = 206,
    SYSCALL_SHIRE_CACHE_BANK_OP = 207,

    // from userspace to supervisor for compute firmware [300-399]
    //SYSCALL_KERNEL_CLEAR = 300,
    SYSCALL_DRAIN_COALESCING_BUFFER_ALT = 301,
    SYSCALL_CACHE_CONTROL_ALT = 302,
    SYSCALL_FLUSH_L3_ALT = 303,
    SYSCALL_SHIRE_CACHE_BANK_OP_ALT = 304,
    SYSCALL_EVICT_L1_ALT = 305,
    SYSCALL_RETURN_FROM_KERNEL = 306,

    // from userspace to supervisor for both master and compute firmwares [400-499]
    SYSCALL_LOG_WRITE = 400,
    SYSCALL_GET_LOG_LEVEL = 401,
    SYSCALL_MESSAGE_SEND = 402
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
