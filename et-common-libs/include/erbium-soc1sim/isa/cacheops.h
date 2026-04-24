/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

/*! \file cacheops.h
    \brief Cache-maintenance ops for the erbium-soc1sim backend.

    Scope: only U-mode-issuable ops live in this header. Ops that
    require M-mode on ET-SoC-1 (set/way evict/flush/lock/unlock,
    cache_invalidate, set_l1_cache_control, whole-L1 evict) live in
    the companion <erbium/isa/cacheops-umode.h>, routed through
    firmware syscalls. Kernels that need any of those include the
    umode header alongside this one.

    cb_drain from etsoc/isa/cacheops.h is intentionally omitted —
    depends on ESR_CACHE / SC_IDX_COP_SM_CTL_USER which are not yet
    defined in erbium's esr_defines.h, AND targets an L3 resource
    that doesn't exist on Erbium anyway.
*/

#ifndef _ERBIUM_ISA_CACHEOPS_H_
#define _ERBIUM_ISA_CACHEOPS_H_

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#include <cinttypes>
#else
#include <inttypes.h>
#endif

#include "erbium/isa/utils.h"   /* FENCE, WAIT_CACHEOPS */

#ifdef __cplusplus
extern "C" {
#endif

/* ---- M-mode-only direct CSR ops -------------------------------- */
/*
 * These mirror the M-mode entry points in <erbium/isa/cacheops.h>
 * for source-compatibility, but are not callable from a U-mode
 * soc1sim kernel. Any reference triggers a hard compile error.
 *
 * U-mode kernels that need to invoke an M-mode cache op must include
 * <erbium/isa/cacheops-umode.h> instead, which routes through a
 * firmware syscall. The two headers are not mixed in the same TU.
 */
#define _SOC1SIM_M_MODE_NOT_HERE                                          \
    "M-mode cache op not callable from soc1sim U-mode kernel; "           \
    "include <erbium/isa/cacheops-umode.h> for the syscall wrapper instead"

extern void evict_sw(uint64_t use_tmask, uint64_t way, uint64_t set, uint64_t num_lines)
    __attribute__((error(_SOC1SIM_M_MODE_NOT_HERE)));

extern void flush_sw(uint64_t use_tmask, uint64_t way, uint64_t set, uint64_t num_lines)
    __attribute__((error(_SOC1SIM_M_MODE_NOT_HERE)));

extern void lock_sw(uint64_t way, uint64_t paddr)
    __attribute__((error(_SOC1SIM_M_MODE_NOT_HERE)));

extern void unlock_sw(uint64_t way, uint64_t set)
    __attribute__((error(_SOC1SIM_M_MODE_NOT_HERE)));

extern void cache_invalidate(uint64_t inval_instr_cache, uint64_t inval_TLBs_and_PTW)
    __attribute__((error(_SOC1SIM_M_MODE_NOT_HERE)));

extern uint64_t get_cache_invalidate(void)
    __attribute__((error(_SOC1SIM_M_MODE_NOT_HERE)));

extern void mcache_control(uint64_t d1_split, uint64_t scp_en,
                           uint64_t cacheop_rate, uint64_t cacheop_max)
    __attribute__((error(_SOC1SIM_M_MODE_NOT_HERE)));

extern void excl_mode(uint64_t val)
    __attribute__((error(_SOC1SIM_M_MODE_NOT_HERE)));

#undef _SOC1SIM_M_MODE_NOT_HERE

/* Destination level encoded in bits [59:58] of the U-mode cache-op
 * CSR. On erbium-soc1sim we always target L2 — the cross-hart
 * shared level within the one neighborhood our kernels run in.
 * Hardcoded here rather than threaded through the API so kernels
 * look identical on native erbium (which has no choice at all).
 *
 * Also consumed by cacheops-umode.h for the syscall-routed ops. */
#define CACHEOP_DST_L2  0x1ULL

/*! \enum l1d_mode
    \brief L1 data cache configuration — matches the encoding
    returned by `get_l1d_mode()` and expected by (indirectly)
    `set_l1_cache_control()` in cacheops-umode.h. */
enum l1d_mode { l1d_shared, l1d_split, l1d_scp };

/* --------------------------------------------------------------- */
/* VA-based ops (U-mode CSRs 0x89f / 0x8bf / 0x81f / 0x8df / 0x8ff) */
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

/*! \brief Tune per-minion cacheop queue parameters. U-mode writeable
 *         (CSR 0x810) so no syscall is needed. For L1 split-mode /
 *         scratchpad-enable (which require M-mode coordination) see
 *         `set_l1_cache_control()` in cacheops-umode.h. */
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

/*! \brief Fence + drain outstanding cacheops, then re-program
 *         ucache_control for scratchpad access. U-mode CSR 0x810. */
static inline __attribute__((always_inline))
void scp(uint64_t warl, uint64_t DEscratchpad)
{
    FENCE;
    WAIT_CACHEOPS;

    uint64_t csr_enc = ((warl & 0x7FFFFFFFFFFFFFFF) << 1) | (DEscratchpad & 0x1);

    __asm__ __volatile__("csrw 0x810, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc) : "x31");
}

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_ISA_CACHEOPS_H_ */
