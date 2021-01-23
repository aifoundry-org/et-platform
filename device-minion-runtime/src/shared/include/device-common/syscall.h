#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

enum {
    // From U-mode to S-mode for compute firmware [300-399]
    SYSCALL_RETURN_FROM_KERNEL = 300,
    SYSCALL_CACHE_CONTROL = 301,
    SYSCALL_FLUSH_L3 = 302,
    SYSCALL_SHIRE_CACHE_BANK_OP = 303,
    SYSCALL_EVICT_L1 = 304,

    // From U-mode to S-mode for both master and compute firmwares [400-499]
    SYSCALL_LOG_WRITE = 400,
    SYSCALL_GET_LOG_LEVEL = 401,
    SYSCALL_GET_MTIME = 402,
    SYSCALL_CONFIGURE_PMCS = 403, // emizan: This should not be used but I am adding it for testing with hardcoded values.
    SYSCALL_SAMPLE_PMCS = 404,
    SYSCALL_RESET_PMCS = 405
};

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

#endif // SYSCALL_H
