#ifndef SYSCALL_H
#define SYSCALL_H

typedef enum{
    SYSCALL_EVICT_L1_TO_L2 = 0,
    SYSCALL_EVICT_L2_TO_L3,
    SYSCALL_INIT_L1,
    SYSCALL_U_MODE,
    SYSCALL_S_MODE,
    SYSCALL_DRAIN_COALESCING_BUFFER,
    SYSCALL_CNT
} syscall_t;

static inline int syscall(syscall_t syscall)
{
    register int ret asm("a1");
    asm volatile("ecall" : "=r" (ret) : "a0" (syscall) : "memory");
    return ret;
}

#endif // SYSCALL_H
