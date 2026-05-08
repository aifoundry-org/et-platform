/***********************************************************************/
/*! \copyright
  Copyright (c) 2026 Ainekko, Co.
  SPDX-License-Identifier: Apache-2.0
*/
/***********************************************************************/
/*! \file syscall.h
    \brief Syscall numbers understood by the etsoc1 firmware.

    The erbium-soc1sim backend runs U-mode kernels on top of the
    etsoc1 firmware (device-minion-runtime's WorkerMinion/MachineMinion).
    These constants mirror the ones in et-common-libs' etsoc/isa/syscall.h
    so a kernel can issue the same ecall contract. The native erbium
    backend will grow its own syscall ABI separately.
*/
/***********************************************************************/

#ifndef _ERBIUM_SOC1SIM_ISA_SYSCALL_H_
#define _ERBIUM_SOC1SIM_ISA_SYSCALL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Range (0-127) dedicated for syscalls allowed from U-Mode. */
#define SYSCALL_UMODE_THRESHOLD             0
#define SYSCALL_CACHE_OPS_EVICT_SW          (SYSCALL_UMODE_THRESHOLD + 1)
#define SYSCALL_CACHE_OPS_FLUSH_SW          (SYSCALL_UMODE_THRESHOLD + 2)
#define SYSCALL_CACHE_OPS_LOCK_SW           (SYSCALL_UMODE_THRESHOLD + 3)
#define SYSCALL_CACHE_OPS_UNLOCK_SW         (SYSCALL_UMODE_THRESHOLD + 4)
#define SYSCALL_CACHE_OPS_INVALIDATE        (SYSCALL_UMODE_THRESHOLD + 5)
#define SYSCALL_CACHE_OPS_EVICT_L1          (SYSCALL_UMODE_THRESHOLD + 6)
#define SYSCALL_SHIRE_CACHE_BANK_OP         (SYSCALL_UMODE_THRESHOLD + 7)
#define SYSCALL_RETURN_FROM_KERNEL          (SYSCALL_UMODE_THRESHOLD + 8)
#define SYSCALL_PMC_SC_SAMPLE               (SYSCALL_UMODE_THRESHOLD + 9)
#define SYSCALL_PMC_MS_SAMPLE               (SYSCALL_UMODE_THRESHOLD + 10)
#define SYSCALL_CACHE_OPS_EVICT_WHOLE_L1_L2 (SYSCALL_UMODE_THRESHOLD + 11)
#define SYSCALL_UMODE_THRESHOLD_LIMIT       127

/* Legacy range (300-399) — S-mode compute-firmware interfaces that
 * WorkerMinion still dispatches when called from U-mode. Only the
 * ones whose behaviour maps onto Erbium semantics are declared
 * here; L3 flush, PMC sampling, etc. are intentionally omitted. */
#define SYSCALL_CACHE_CONTROL               301u /* set_l1_cache_control(d1_split, scp_en) */

/* SYSCALL error codes */
#define SYSCALL_SUCCESS    0
#define SYSCALL_INVALID_ID -1

/* Kernel return types (a2 argument to SYSCALL_RETURN_FROM_KERNEL).
   Must stay in sync with the etsoc1 firmware. */
#define KERNEL_RETURN_SUCCESS      0
#define KERNEL_RETURN_SELF_ABORT   1
#define KERNEL_RETURN_SYSTEM_ABORT 2
#define KERNEL_RETURN_EXCEPTION    3

#ifndef __ASSEMBLER__

#include <stdint.h>

static inline __attribute__((always_inline)) int64_t syscall(
    uint64_t syscall, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    register uint64_t a0 asm("a0") = syscall;
    register uint64_t a1 asm("a1") = arg1;
    register uint64_t a2 asm("a2") = arg2;
    register uint64_t a3 asm("a3") = arg3;

    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3) : "memory");

    return (int64_t)a0;
}

#endif /* __ASSEMBLER__ */

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_SOC1SIM_ISA_SYSCALL_H_ */
