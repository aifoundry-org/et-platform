/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

/*! \file cacheops-umode.h
    \brief U-mode entry points for cache-maintenance ops on the
    erbium-soc1sim backend.

    Self-contained: kernels that only run in U-mode include this
    header and get the full surface (VA-based ops issued via direct
    CSR writes + syscall wrappers for ops that require M-mode on
    ET-SoC-1). Companion <erbium/isa/cacheops.h> targets M-mode
    callers with direct CSR access to the privileged ops; the two
    headers ship in different packages and are not mixed in the
    same translation unit.

    Destination level: hidden from every signature — soc1sim pins
    it to `CACHEOP_DST_L2` internally. On real Erbium the same
    encoding resolves to "memory" (since erbium has no L2/L3 cache),
    matching native erbium's hardcoded `CACHEOP_DST_MEM`.
*/

#ifndef _ERBIUM_SOC1SIM_ISA_CACHEOPS_UMODE_H_
#define _ERBIUM_SOC1SIM_ISA_CACHEOPS_UMODE_H_

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#include <cinttypes>
#else
#include <inttypes.h>
#endif

#include "erbium/isa/utils.h"     /* FENCE, WAIT_CACHEOPS */
#include "erbium/isa/syscall.h"   /* SYSCALL_* numbers + syscall() */

#ifdef __cplusplus
extern "C" {
#endif

/* Destination level encoded in bits [59:58] of the cache-op CSRs.
 * Mirrors the value defined in <erbium/isa/cacheops.h>; duplicated
 * here so this header is self-contained — cacheops.h and
 * cacheops-umode.h ship in different consumers and are not included
 * in the same TU. */
#define CACHEOP_DST_L2  0x1ULL

/*! \enum l1d_mode
    \brief L1 data cache configuration. */
enum l1d_mode { l1d_shared, l1d_split, l1d_scp };

/* --------------------------------------------------------------- */
/* VA-based U-mode ops (CSRs 0x89f / 0x8bf / 0x81f / 0x8df / 0x8ff) */
/* --------------------------------------------------------------- */

static inline __attribute__((always_inline))
void evict_va(uint64_t use_tmask, uint64_t addr,
              uint64_t num_lines, uint64_t stride, uint64_t id)
{
    uint64_t csr_enc = ((use_tmask & 1) << 63) |
                       (CACHEOP_DST_L2 << 58) |
                       (addr & 0xFFFFFFFFFFC0ULL) |
                       (num_lines & 0xF);

    register uint64_t x31_enc asm("x31") =
        (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

    __asm__ __volatile__("csrw 0x89f, %[csr_enc]\n"
                         :
                         : [x31_enc] "r"(x31_enc),
                           [csr_enc] "r"(csr_enc));
}

static inline __attribute__((always_inline))
void evict_va_all(uint64_t use_tmask, uint64_t addr,
                  uint64_t num_lines, uint64_t stride, uint64_t id)
{
    while (num_lines > 15)
    {
        evict_va(use_tmask, addr, 15, stride, id);
        addr += (stride * 16);
        num_lines -= 15;
    }
    evict_va(use_tmask, addr, num_lines, stride, id);
}

static inline __attribute__((always_inline))
void evict(volatile const void *const address, uint64_t size)
{
    evict_va_all(0, (uint64_t)address,
                 (((uint64_t)address & 0x3F) + size) >> 6, 64, 0);
}

static inline __attribute__((always_inline))
void flush_va(uint64_t use_tmask, uint64_t addr,
              uint64_t num_lines, uint64_t stride, uint64_t id)
{
    uint64_t csr_enc = ((use_tmask & 1) << 63) |
                       (CACHEOP_DST_L2 << 58) |
                       (addr & 0xFFFFFFFFFFC0ULL) |
                       (num_lines & 0xF);

    register uint64_t x31_enc asm("x31") =
        (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

    __asm__ __volatile__("csrw 0x8bf, %[csr_enc]\n"
                         :
                         : [x31_enc] "r"(x31_enc),
                           [csr_enc] "r"(csr_enc));
}

static inline __attribute__((always_inline))
void prefetch_va(uint64_t use_tmask, uint64_t addr,
                 uint64_t num_lines, uint64_t stride, uint64_t id)
{
    uint64_t csr_enc = ((use_tmask & 1) << 63) |
                       (CACHEOP_DST_L2 << 58) |
                       (addr & 0xFFFFFFFFFFC0ULL) |
                       (num_lines & 0xF);

    register uint64_t x31_enc asm("x31") =
        (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

    __asm__ __volatile__("csrw 0x81f, %[csr_enc]\n"
                         :
                         : [x31_enc] "r"(x31_enc),
                           [csr_enc] "r"(csr_enc));
}

static inline __attribute__((always_inline))
void lock_va(uint64_t use_tmask, uint64_t addr, uint64_t num_lines,
             uint64_t stride, uint64_t id)
{
    uint64_t csr_enc = ((use_tmask & 1) << 63) |
                       (addr & 0xFFFFFFFFFFC0ULL) | (num_lines & 0xF);

    register uint64_t x31_enc asm("x31") =
        (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

    __asm__ __volatile__("csrw 0x8df, %[csr_enc]\n"
                         :
                         : [x31_enc] "r"(x31_enc),
                           [csr_enc] "r"(csr_enc));
}

static inline __attribute__((always_inline))
void unlock_va(uint64_t use_tmask, uint64_t addr, uint64_t num_lines,
               uint64_t stride, uint64_t id)
{
    uint64_t csr_enc = ((use_tmask & 1) << 63) |
                       (addr & 0xFFFFFFFFFFC0ULL) | (num_lines & 0xF);

    register uint64_t x31_enc asm("x31") =
        (stride & 0xFFFFFFFFFFC0ULL) | (id & 0x1);

    __asm__ __volatile__("csrw 0x8ff, %[csr_enc]\n"
                         :
                         : [x31_enc] "r"(x31_enc),
                           [csr_enc] "r"(csr_enc));
}

/* --------------------------------------------------------------- */
/* U-mode-accessible control (CSR 0x810)                            */
/* --------------------------------------------------------------- */

static inline __attribute__((always_inline))
void ucache_control(uint64_t scp_en, uint64_t cacheop_rate, uint64_t cacheop_max)
{
    uint64_t csr_enc = ((cacheop_max  & 0x1F) << 6) |
                       ((cacheop_rate & 0x7)  << 2) |
                       ((scp_en       & 0x1)  << 1);

    __asm__ __volatile__("csrw 0x810, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc) : "x31");
}

static inline __attribute__((always_inline))
enum l1d_mode get_l1d_mode(void)
{
    uint64_t csr_enc;
    __asm__ __volatile__("csrr %[csr_enc], 0x810\n" : [csr_enc] "=r"(csr_enc) : :);

    if ((csr_enc & 0x3) == 0x3)
        return l1d_scp;
    return ((csr_enc & 0x3) == 0x1) ? l1d_split : l1d_shared;
}

static inline __attribute__((always_inline))
void scp(uint64_t warl, uint64_t DEscratchpad)
{
    FENCE;
    WAIT_CACHEOPS;

    uint64_t csr_enc = ((warl & 0x7FFFFFFFFFFFFFFF) << 1) | (DEscratchpad & 0x1);

    __asm__ __volatile__("csrw 0x810, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc) : "x31");
}

/* --------------------------------------------------------------- */
/* Set/way ops (M-mode CSRs 0x7f9 / 0x7fb / 0x7fd / 0x7ff)          */
/* Routed through the WorkerMinion firmware syscall dispatcher.     */
/* --------------------------------------------------------------- */

/*! \brief Evict a specific set/way from this hart's L1. num_lines
 *         is 0..15 (inclusive) = 1..16 lines. */
static inline __attribute__((always_inline)) int64_t
evict_sw(uint64_t use_tmask, uint64_t way, uint64_t set, uint64_t num_lines)
{
    uint64_t csr_enc = ((use_tmask      & 1ULL)  << 63) |
                       ((CACHEOP_DST_L2 & 0x3ULL) << 58) |
                       ((set            & 0xFULL) << 14) |
                       ((way            & 0x3ULL) << 6)  |
                       (num_lines       & 0xFULL);
    return syscall(SYSCALL_CACHE_OPS_EVICT_SW, csr_enc, 0, 0);
}

/*! \brief Writeback a specific set/way if dirty; line stays in L1. */
static inline __attribute__((always_inline)) int64_t
flush_sw(uint64_t use_tmask, uint64_t way, uint64_t set, uint64_t num_lines)
{
    uint64_t csr_enc = ((use_tmask      & 1ULL)  << 63) |
                       ((CACHEOP_DST_L2 & 0x3ULL) << 58) |
                       ((set            & 0xFULL) << 14) |
                       ((way            & 0x3ULL) << 6)  |
                       (num_lines       & 0xFULL);
    return syscall(SYSCALL_CACHE_OPS_FLUSH_SW, csr_enc, 0, 0);
}

/*! \brief Hard-lock a physical address into a specific L1 way. */
static inline __attribute__((always_inline)) int64_t
lock_sw(uint64_t way, uint64_t paddr)
{
    uint64_t csr_enc = ((way & 0x3ULL) << 55) |
                       (paddr & 0xFFFFFFFFC0ULL);
    return syscall(SYSCALL_CACHE_OPS_LOCK_SW, csr_enc, 0, 0);
}

/*! \brief Hard-unlock a specific set/way. */
static inline __attribute__((always_inline)) int64_t
unlock_sw(uint64_t way, uint64_t set)
{
    uint64_t csr_enc = ((way & 0xFFULL) << 55) |
                       ((set & 0xFULL)  << 6);
    return syscall(SYSCALL_CACHE_OPS_UNLOCK_SW, csr_enc, 0, 0);
}

/* --------------------------------------------------------------- */
/* Invalidate / control (M-mode CSRs 0x7d0, 0x7e0)                  */
/* --------------------------------------------------------------- */

/*! \brief Invalidate L1 I-cache and/or TLBs + PTW. Either flag may
 *         be 0 to leave the corresponding structure alone. */
static inline __attribute__((always_inline)) int64_t
cache_invalidate(uint64_t inval_instr_cache, uint64_t inval_TLBs_and_PTW)
{
    uint64_t csr_enc = (inval_TLBs_and_PTW & 1ULL) |
                       ((inval_instr_cache & 1ULL) << 1);
    return syscall(SYSCALL_CACHE_OPS_INVALIDATE, csr_enc, 0, 0);
}

/*! \brief Re-configure this minion's L1 data cache: `d1_split`
 *         selects shared (0) vs split (1) mode; `scp_en` enables
 *         the scratchpad portion (requires split). */
static inline __attribute__((always_inline)) int64_t
set_l1_cache_control(uint64_t d1_split, uint64_t scp_en)
{
    return syscall(SYSCALL_CACHE_CONTROL, d1_split & 1ULL, scp_en & 1ULL, 0);
}

/* --------------------------------------------------------------- */
/* Whole-L1 evict                                                   */
/* --------------------------------------------------------------- */

/*! \brief Evict every currently-active cache line from this hart's
 *         L1, gated by the TensorMask CSR if requested. */
static inline __attribute__((always_inline)) int64_t
evict_l1(uint64_t use_tmask)
{
    return syscall(SYSCALL_CACHE_OPS_EVICT_L1,
                   use_tmask & 1ULL,
                   CACHEOP_DST_L2, 0);
}

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_SOC1SIM_ISA_CACHEOPS_UMODE_H_ */
