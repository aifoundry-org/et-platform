#ifndef SYSCALL_H
#define SYSCALL_H

/* SYSCALL IDs for syscalls from U-Mode */
/* Range (0-127) dedicated for syscalls allowed from U-Mode */
#define SYSCALL_UMODE_THRESHOLD         0
#define SYSCALL_CACHE_OPS_EVICT_SW      (SYSCALL_UMODE_THRESHOLD + 1)
#define SYSCALL_CACHE_OPS_FLUSH_SW      (SYSCALL_UMODE_THRESHOLD + 2)
#define SYSCALL_CACHE_OPS_LOCK_SW       (SYSCALL_UMODE_THRESHOLD + 3)
#define SYSCALL_CACHE_OPS_UNLOCK_SW     (SYSCALL_UMODE_THRESHOLD + 4)
#define SYSCALL_CACHE_OPS_INVALIDATE    (SYSCALL_UMODE_THRESHOLD + 5)
#define SYSCALL_SHIRE_CACHE_BANK_OP     (SYSCALL_UMODE_THRESHOLD + 6)
#define SYSCALL_RETURN_FROM_KERNEL      (SYSCALL_UMODE_THRESHOLD + 7)
#define SYSCALL_UMODE_THRESHOLD_LIMIT   127

/* SYSCALL IDs for syscalls from U-Mode */
/* Range (128-511) dedicated for syscalls allowed from S-Mode */
#define SYSCALL_SMODE_THRESHOLD         128
#define SYSCALL_SMODE_THRESHOLD_LIMIT   512

/* SYSCALL error codes */
#define SYSCALL_SUCCESS                 0
#define SYSCALL_INVALID_ID              -1

/* Kernel return types. Must be kept synced with FW.
TODO: Need a separate header for it? */
#define KERNEL_RETURN_SUCCESS            0
#define KERNEL_RETURN_SELF_ABORT         1
#define KERNEL_RETURN_SYSTEM_ABORT       2
#define KERNEL_RETURN_EXCEPTION          3

#ifndef __ASSEMBLER__

#include <stdint.h>

static inline __attribute__((always_inline)) int64_t syscall(uint64_t syscall, uint64_t arg1,
                                                             uint64_t arg2, uint64_t arg3)
{
    register uint64_t a0 asm("a0") = syscall;
    register uint64_t a1 asm("a1") = arg1;
    register uint64_t a2 asm("a2") = arg2;
    register uint64_t a3 asm("a3") = arg3;

    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3) : "memory");

    return (int64_t)a0;
}

#endif

#endif // SYSCALL_H
