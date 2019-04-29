#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

typedef enum {
    SYSCALL_PRE_KERNEL_SETUP = 0,
    SYSCALL_POST_KERNEL_CLEANUP,
    SYSCALL_EVICT_L1,
    SYSCALL_EVICT_L2,
    SYSCALL_INIT_L1,
    SYSCALL_DRAIN_COALESCING_BUFFER,
    SYSCALL_RETURN_FROM_KERNEL,
    SYSCALL_ENABLE_THREAD1,
    SYSCALL_BROADCAST,
    SYSCALL_IPI_TRIGGER
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
