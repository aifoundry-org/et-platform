/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

/*! \file cacheops.h
    \brief Cache-maintenance ops for the native erbium backend.

    Native erbium has only per-hart L1; main memory (MRAM) sits at
    what would be L2 on ET-SoC-1. There is no choice of destination
    level, so the U-mode cache ops on this backend take no `dst`
    parameter — everything evicts/flushes to memory by definition.
    Kernels that need matching soc1sim semantics go through the
    same parameterless API (see include/erbium-soc1sim/erbium/isa/
    cacheops.h).

    TODO(m-mode): the set/way ops (0x7f9 evict_sw, 0x7fb flush_sw,
    0x7fd lock_sw, 0x7ff unlock_sw) and the control CSRs (0x7d0
    cache_invalidate, 0x7d3 excl_mode, 0x7e0 mcache_control) require
    M-mode. Erbium kernels run in U-mode; these will trap as-is.
    A later pass will either syscall-wrap or drop them. Left intact
    so the port from etsoc/isa/cacheops.h stays traceable.

    TODO(real-hw): the CSR bit encoding below reuses the etsoc
    layout and programs the dst field to CACHEOP_DST_MEM. Once the
    real erbium cache-op encoding is finalised in the HAL, reconcile.
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

/* Native erbium has only L1 and memory, so the U-mode cache ops'
 * 2-bit dst field (bits [59:58] of the CSR encoding) is always
 * programmed to "memory". Kept as a named constant so the three
 * call sites below don't repeat a bare 0x3. Matches the etsoc
 * to_Mem value, so the soc1sim backend's 0x89f/0x8bf/0x81f CSRs
 * accept the same encoding when the erbium API is fronted by
 * the soc1sim RTL. */
#define CACHEOP_DST_MEM  0x3ULL

/*! \enum l1d_mode
    \brief L1 data cache configuration. */
enum l1d_mode { l1d_shared, l1d_split, l1d_scp };

/* --------------------------------------------------------------- */
/* Set/way ops (require M-mode today; see file header)              */
/* --------------------------------------------------------------- */

static inline __attribute__((always_inline))
void evict_sw(uint64_t use_tmask, uint64_t way, uint64_t set, uint64_t num_lines)
{
    uint64_t csr_enc = ((use_tmask & 1) << 63) | (CACHEOP_DST_MEM << 58) |
                       ((set & 0xF) << 14) | ((way & 0x3) << 6) |
                       (num_lines & 0xF);

    __asm__ __volatile__("csrw 0x7f9, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc));
}

static inline __attribute__((always_inline))
void flush_sw(uint64_t use_tmask, uint64_t way, uint64_t set, uint64_t num_lines)
{
    uint64_t csr_enc = ((use_tmask & 1) << 63) | (CACHEOP_DST_MEM << 58) |
                       ((set & 0xF) << 14) | ((way & 0x3) << 6) |
                       (num_lines & 0xF);

    __asm__ __volatile__("csrw 0x7fb, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc));
}

static inline __attribute__((always_inline))
void lock_sw(uint64_t way, uint64_t paddr)
{
    uint64_t csr_enc = ((way & 0x3) << 55) | (paddr & 0xFFFFFFFFC0ULL);

    __asm__ __volatile__("csrw 0x7fd, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc));
}

static inline __attribute__((always_inline))
void unlock_sw(uint64_t way, uint64_t set)
{
    uint64_t csr_enc = ((way & 0xFF) << 55) | ((set & 0xF) << 6);

    __asm__ __volatile__("csrw 0x7ff, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc));
}

/* --------------------------------------------------------------- */
/* VA-based ops (available in U-mode)                               */
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
/* Invalidate / control                                             */
/* --------------------------------------------------------------- */

static inline __attribute__((always_inline))
void cache_invalidate(uint64_t inval_instr_cache, uint64_t inval_TLBs_and_PTW)
{
    uint64_t csr_enc = (inval_TLBs_and_PTW & 1) | ((inval_instr_cache & 1) << 1);

    __asm__ __volatile__("csrw 0x7d0, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc));
}

static inline __attribute__((always_inline))
uint64_t get_cache_invalidate(void)
{
    uint64_t csr_enc;
    __asm__ __volatile__("csrr %[csr_enc], 0x7d0\n" : [csr_enc] "=r"(csr_enc) : :);
    return csr_enc;
}

static inline __attribute__((always_inline))
void mcache_control(uint64_t d1_split, uint64_t scp_en,
                    uint64_t cacheop_rate, uint64_t cacheop_max)
{
    uint64_t csr_enc = ((cacheop_max & 0x1F) << 6) | ((cacheop_rate & 0x7) << 2) |
                       ((scp_en & 0x1) << 1) | ((d1_split & 0x1) << 0);

    __asm__ __volatile__("csrw 0x7e0, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc) : "x31");
}

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
void excl_mode(uint64_t val)
{
    __asm__ __volatile__("csrw 0x7d3, %[csr_enc]\n" : : [csr_enc] "r"(val) : "x31");
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

#endif /* _ERBIUM_ISA_CACHEOPS_H_ */
