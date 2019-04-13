#ifndef SYSCALL_H
#define SYSCALL_H

typedef enum {
    SYSCALL_EVICT_L1_TO_L2 = 0,
    SYSCALL_EVICT_L2_TO_L3,
    SYSCALL_INIT_L1,
    SYSCALL_DRAIN_COALESCING_BUFFER,
    SYSCALL_LAUNCH_KERNEL,
    SYSCALL_RETURN_FROM_KERNEL,
    SYSCALL_ENABLE_THREAD1,
    SYSCALL_CNT
} syscall_t;

static inline __attribute__((always_inline)) int syscall(syscall_t syscall)
{
    register int number asm("a0") = syscall;
    register int ret asm("a1");

    asm volatile("ecall" : "=r" (ret) : "r" (number) : "memory");

    return ret;
}

#endif // SYSCALL_H
