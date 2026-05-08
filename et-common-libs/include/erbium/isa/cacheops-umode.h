/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

/*! \file cacheops-umode.h
    \brief U-mode cache-maintenance API for native erbium.

    This is the header U-mode kernels include for cache ops.
    It provides the VA-based ops (evict_va, flush_va, prefetch_va,
    lock_va, unlock_va) and the U-mode control CSRs (ucache_control,
    get_l1d_mode, scp) — all accessible from U-mode via direct CSR
    writes.

    Native erbium has only per-hart L1 and MRAM, so the dst field
    is always hardcoded to CACHEOP_DST_MEM (0x3).

    M-mode ops (evict_sw, flush_sw, lock_sw, unlock_sw,
    cache_invalidate, excl_mode, mcache_control) are NOT provided
    here — they live in cacheops.h and require M-mode privilege.
    Once a syscall bridge is implemented, this header will grow
    wrappers matching the soc1sim cacheops-umode.h surface.
*/

#ifndef _ERBIUM_ISA_CACHEOPS_UMODE_H_
#define _ERBIUM_ISA_CACHEOPS_UMODE_H_

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#include <cinttypes>
#else
#include <inttypes.h>
#endif

#include "erbium/isa/utils.h"   /* FENCE, WAIT_CACHEOPS */

#ifdef __cplusplus
extern "C" {
#endif

/* ---- M-mode-syscall entry points (not implemented on erbium) ----- */
/*
 * The soc1sim backend exposes M-mode cache ops to U-mode kernels via
 * firmware syscalls (see erbium-soc1sim/isa/cacheops-umode.h). Native
 * erbium has no M-mode firmware shim yet, so the matching surface is
 * declared here only as compile-time-error stubs — kernels that want
 * to be source-portable across both backends compile fine, and any
 * actual call on the native-erbium build fails with a clear message.
 */
#define _ERBIUM_M_MODE_SYSCALL_NOT_IMPL                                     \
    "M-mode cache op syscall not implemented on native erbium; "            \
    "include <erbium/isa/cacheops.h> for direct CSR access from M-mode "    \
    "code, or guard the call with a per-backend #ifdef"

extern int64_t evict_sw(uint64_t use_tmask, uint64_t way, uint64_t set, uint64_t num_lines)
    __attribute__((error(_ERBIUM_M_MODE_SYSCALL_NOT_IMPL)));

extern int64_t flush_sw(uint64_t use_tmask, uint64_t way, uint64_t set, uint64_t num_lines)
    __attribute__((error(_ERBIUM_M_MODE_SYSCALL_NOT_IMPL)));

extern int64_t lock_sw(uint64_t way, uint64_t paddr)
    __attribute__((error(_ERBIUM_M_MODE_SYSCALL_NOT_IMPL)));

extern int64_t unlock_sw(uint64_t way, uint64_t set)
    __attribute__((error(_ERBIUM_M_MODE_SYSCALL_NOT_IMPL)));

extern int64_t cache_invalidate(uint64_t inval_instr_cache, uint64_t inval_TLBs_and_PTW)
    __attribute__((error(_ERBIUM_M_MODE_SYSCALL_NOT_IMPL)));

extern int64_t set_l1_cache_control(uint64_t d1_split, uint64_t scp_en)
    __attribute__((error(_ERBIUM_M_MODE_SYSCALL_NOT_IMPL)));

extern int64_t evict_l1(uint64_t use_tmask)
    __attribute__((error(_ERBIUM_M_MODE_SYSCALL_NOT_IMPL)));

#undef _ERBIUM_M_MODE_SYSCALL_NOT_IMPL

/* Destination level for VA-based ops.  Native erbium has only L1
 * and MRAM, so everything targets memory. */
#define CACHEOP_DST_MEM  0x3ULL

/*! \enum l1d_mode
    \brief L1 data cache configuration. */
enum l1d_mode { l1d_shared, l1d_split, l1d_scp };

/* --------------------------------------------------------------- */
/* VA-based ops (U-mode CSRs 0x89f, 0x8bf, 0x81f, 0x8df, 0x8ff)    */
/* --------------------------------------------------------------- */

static inline __attribute__((always_inline))
void evict_va(uint64_t use_tmask, uint64_t addr,
              uint64_t num_lines, uint64_t stride, uint64_t id)
{
    uint64_t csr_enc = ((use_tmask & 1) << 63) |
                       (CACHEOP_DST_MEM << 58) |
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
                       (CACHEOP_DST_MEM << 58) |
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
                       (CACHEOP_DST_MEM << 58) |
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
/* U-mode control CSRs (0x810)                                      */
/* --------------------------------------------------------------- */

static inline __attribute__((always_inline))
void ucache_control(uint64_t scp_en, uint64_t cacheop_rate, uint64_t cacheop_max)
{
    uint64_t csr_enc = ((cacheop_max & 0x1F) << 6) | ((cacheop_rate & 0x7) << 2) |
                       ((scp_en & 0x1) << 1);

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

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_ISA_CACHEOPS_UMODE_H_ */
