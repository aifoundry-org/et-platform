#ifndef SYSCALL_H
#define SYSCALL_H

// From U-mode to S-mode for compute firmware [300-399]
#define SYSCALL_RETURN_FROM_KERNEL   300
#define SYSCALL_CACHE_CONTROL        301
#define SYSCALL_FLUSH_L3             302
#define SYSCALL_SHIRE_CACHE_BANK_OP  303
#define SYSCALL_EVICT_L1             304
#define SYSCALL_LOG_WRITE            305
#define SYSCALL_GET_LOG_LEVEL        306
#define SYSCALL_GET_MTIME            307
#define SYSCALL_CONFIGURE_PMCS       308 // emizan: This should not be used but I am adding it for testing with hardcoded values.
#define SYSCALL_SAMPLE_PMCS          309
#define SYSCALL_RESET_PMCS           310

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
