/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

/*
 * ET-SoC-1 ESR address composition for the erbium-soc1sim backend.
 *
 * Public API mirrors the native-erbium side (esr_addr / esr_read_u64
 * / esr_write_u64 + ESR_SR_HART / _NEIGH / _CPU); only the address
 * layout differs:
 *
 *     [32]     1    ESR space marker
 *     [31:30]  PP        (privilege: U=0, S=1, D=2, M=3)
 *     [29:22]  shire id  (8 bits — internally always forced to 0xFF
 *                         "local shire" so kernels can keep writing
 *                         THIS_SHIRE == 0 from <erbium/isa/hart.h>)
 *     [21:0]   sub-region | byte offset within block
 *
 * The shire-argument substitution preserves the kernel-facing
 * abstraction ("there is one shire, ID 0") while addressing the
 * actual ET-SoC-1 hardware (which has many shires; the caller's
 * own shire is encoded as 0xFF, not as its absolute shire id).
 */

#ifndef _ERBIUM_SOC1SIM_ISA_ESR_DEFINES_H_
#define _ERBIUM_SOC1SIM_ISA_ESR_DEFINES_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASSEMBLER__
#include <inttypes.h>
#endif

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

/* ---- ET-SoC-1 ESR address composition ------------------------ */

#define ESR_REGION             0x0100000000ULL    /* ESR space marker bit [32] */
#define ESR_REGION_PROT_SHIFT  30                 /* PP        [31:30] */
#define ESR_REGION_SHIRE_SHIFT 22                 /* shire id  [29:22] */

/* Sub-region bases inside the 22-bit offset field [21:0]. Match the
 * native-erbium ESR_SR_* constants so the kernel-facing API is
 * uniform across backends. */
#define ESR_SR_HART     0x000000ULL    /* Hart / RVTIMER sub-region */
#define ESR_SR_NEIGH    0x100000ULL    /* Neighborhood sub-region */
#define ESR_SR_CPU      0x340000ULL    /* CPU / shire sub-region (FCC, FLB, IPI, ...) */

/* Build an ESR address from (pp, subregion, byte_offset). Targets
 * the caller's own shire — the shire field is unconditionally 0xFF
 * (etsoc local-shire magic), so kernels writing THIS_SHIRE == 0
 * reach the caller's actual shire on the underlying ET-SoC-1
 * hardware regardless of which shire WorkerMinion picked. */
#define ESR_ADDR(pp, subregion, byte_offset)                                 \
    ((ESR_REGION) |                                                          \
     ((uint64_t)((pp) & 0x3ULL) << ESR_REGION_PROT_SHIFT) |                  \
     ((uint64_t)0xFFULL         << ESR_REGION_SHIRE_SHIFT) |                 \
     ((uint64_t)((subregion) & 0x3FFFFFULL)) |                               \
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
    return *(volatile uint64_t *)(uintptr_t)esr_addr(pp, subregion, offset);
}

static inline __attribute__((always_inline))
void esr_write_u64(uint32_t pp, uint32_t subregion, uint32_t offset, uint64_t val)
{
    *(volatile uint64_t *)(uintptr_t)esr_addr(pp, subregion, offset) = val;
}

#endif /* !__ASSEMBLER__ */

/* ---- thread identifiers ---------------------------- */
/* (THIS_SHIRE already lives in <erbium/isa/hart.h> == 0; the soc1sim
 *  shire-substitution above maps that to the caller's own shire on
 *  the actual ET-SoC-1 hardware.) */

#define THREAD_0 0
#define THREAD_1 1

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_SOC1SIM_ISA_ESR_DEFINES_H_ */
