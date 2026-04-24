/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

/*
 * Erbium ESR (Esperanto Special Register) address layout.
 *
 * Address bit layout:
 *     [31]     1    ESR space marker (set by region base below)
 *     [30:24]  shire id  (always 0 on erbium; exactly one shire)
 *     [23:22]  PP        (privilege: U=0, S=1, D=2, M=3)
 *     [21:0]   sub-region base | byte offset within block
 *
 * Sub-region constants (ESR_SR_*) and the per-register byte offsets
 * come from the HAL hwinc headers; this file just composes them via
 * esr_addr() / esr_read_u64() / esr_write_u64() helpers.
 */

#ifndef _ERBIUM_ISA_ESR_DEFINES_H_
#define _ERBIUM_ISA_ESR_DEFINES_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASSEMBLER__
#include <inttypes.h>
#endif

#include "hwinc/top.h"   /* ERBIUM_TOP_CPU_REGISTERS_BASE */
#include "hwinc/esr.h"   /* per-register *_BYTE_OFFSET symbols */

/* ---- privilege (PP) constants -------------------------------- */

#ifndef PRV_U
#ifdef __ASSEMBLER__
#define PRV_U 0
#else
#define PRV_U 0ull
#endif
#endif

#ifndef PRV_S
#ifdef __ASSEMBLER__
#define PRV_S 1
#else
#define PRV_S 1ull
#endif
#endif

#ifndef PRV_D
#ifdef __ASSEMBLER__
#define PRV_D 2
#else
#define PRV_D 2ull
#endif
#endif

#ifndef PRV_M
#ifdef __ASSEMBLER__
#define PRV_M 3
#else
#define PRV_M 3ull
#endif
#endif

/* ---- erbium ESR address composition -------------------------- */

#define ESR_REGION             ERBIUM_TOP_CPU_REGISTERS_BASE   /* ESR space marker bit [31] */

#define ESR_REGION_PROT_SHIFT  22              /* PP        [23:22] */
#define ESR_REGION_SHIRE_SHIFT 24              /* shire id  [30:24] */

/* Sub-region bases within the 22-bit offset field [21:0]. Pass one
 * of these as the `subregion` argument to esr_addr/read/write; the
 * PP argument selects User/Supervisor/Debug/Machine variant of the
 * same block (e.g. User_cpu vs Machine_cpu both live at ESR_SR_CPU,
 * differentiated by PP=PRV_U vs PRV_M). */
#define ESR_SR_HART            0x000000ULL     /* Hart sub-region */
#define ESR_SR_NEIGH           0x100000ULL     /* Neighborhood sub-region (MPROT, ...) */
#define ESR_SR_CPU             0x340000ULL     /* CPU sub-region (FCC, FLB, IPI, ...) */

/* Build an ESR address from (pp, subregion, byte_offset). Targets
 * the caller's own shire (THIS_SHIRE = 0 — erbium has exactly one
 * shire). Macro form so it's usable from assembler. */
#define ESR_ADDR(pp, subregion, byte_offset)                     \
    ((ESR_REGION) |                                              \
     ((uint64_t)((pp)    & 0x3ULL)  << ESR_REGION_PROT_SHIFT) |  \
     ((uint64_t)((subregion) & 0x3FFFFFULL)) |                   \
     ((uint64_t)((byte_offset) & 0xFFFFULL)))

#ifndef __ASSEMBLER__

static inline __attribute__((always_inline))
uint64_t esr_addr(uint32_t pp, uint32_t subregion, uint32_t offset)
{
    return ESR_ADDR(pp, subregion, offset);
}

static inline __attribute__((always_inline))
uint64_t esr_read_u64(uint32_t pp, uint32_t subregion, uint32_t offset)
{
    return *(volatile uint64_t *)esr_addr(pp, subregion, offset);
}

static inline __attribute__((always_inline))
void esr_write_u64(uint32_t pp, uint32_t subregion, uint32_t offset, uint64_t val)
{
    *(volatile uint64_t *)esr_addr(pp, subregion, offset) = val;
}

#endif /* !__ASSEMBLER__ */

/* ---- thread identifiers ---------------------------- */
/* (THIS_SHIRE already lives in <erbium/isa/hart.h> == 0.) */

#define THREAD_0 0
#define THREAD_1 1

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_ISA_ESR_DEFINES_H_ */
