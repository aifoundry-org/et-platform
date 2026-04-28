/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-----------------------------------------------------------------------*/

/* Erbium PLIC driver. Spec-conformant RISC-V PLIC at
 * ERBIUM_TOP_PLIC_BASE; the per-register naming differs across
 * platforms but the layout (priority/pending/enable/threshold/claim)
 * is universal, so this driver uses spec offsets rather than per-
 * register hwinc symbols. */

#ifndef _ERBIUM_DRIVERS_PLIC_H_
#define _ERBIUM_DRIVERS_PLIC_H_

#include <stdint.h>

#include "common/mmio.h"
#include "hwinc/top.h"   /* ERBIUM_TOP_PLIC_BASE */

#ifdef __cplusplus
extern "C" {
#endif

/* RISC-V PLIC spec offsets, relative to PLIC base. */
#define PLIC_PRIORITY_BASE   0x000000ul
#define PLIC_PENDING_BASE    0x001000ul
#define PLIC_ENABLE_BASE     0x002000ul
#define PLIC_THRESHOLD_BASE  0x200000ul
#define PLIC_CLAIM_BASE      0x200004ul

#define PLIC_ENABLE_STRIDE   0x80ul     /* per-context enable bitmap stride */
#define PLIC_CTX_STRIDE      0x1000ul   /* per-context threshold/claim stride */

static inline __attribute__((always_inline))
uint32_t plic_read_priority(uint32_t src)
{
    return reg_read32(ERBIUM_TOP_PLIC_BASE + PLIC_PRIORITY_BASE + src * 4);
}

static inline __attribute__((always_inline))
void plic_write_priority(uint32_t src, uint32_t val)
{
    reg_write32(ERBIUM_TOP_PLIC_BASE + PLIC_PRIORITY_BASE + src * 4, val);
}

static inline __attribute__((always_inline))
uint32_t plic_read_pending(uint32_t word)
{
    return reg_read32(ERBIUM_TOP_PLIC_BASE + PLIC_PENDING_BASE + word * 4);
}

static inline __attribute__((always_inline))
uint32_t plic_read_enable(uint32_t ctx, uint32_t word)
{
    return reg_read32(ERBIUM_TOP_PLIC_BASE + PLIC_ENABLE_BASE
                      + ctx * PLIC_ENABLE_STRIDE + word * 4);
}

static inline __attribute__((always_inline))
void plic_write_enable(uint32_t ctx, uint32_t word, uint32_t val)
{
    reg_write32(ERBIUM_TOP_PLIC_BASE + PLIC_ENABLE_BASE
                + ctx * PLIC_ENABLE_STRIDE + word * 4, val);
}

static inline __attribute__((always_inline))
uint32_t plic_read_threshold(uint32_t ctx)
{
    return reg_read32(ERBIUM_TOP_PLIC_BASE + PLIC_THRESHOLD_BASE
                      + ctx * PLIC_CTX_STRIDE);
}

static inline __attribute__((always_inline))
void plic_write_threshold(uint32_t ctx, uint32_t val)
{
    reg_write32(ERBIUM_TOP_PLIC_BASE + PLIC_THRESHOLD_BASE
                + ctx * PLIC_CTX_STRIDE, val);
}

static inline __attribute__((always_inline))
uint32_t plic_claim(uint32_t ctx)
{
    return reg_read32(ERBIUM_TOP_PLIC_BASE + PLIC_CLAIM_BASE
                      + ctx * PLIC_CTX_STRIDE);
}

static inline __attribute__((always_inline))
void plic_complete(uint32_t ctx, uint32_t src)
{
    reg_write32(ERBIUM_TOP_PLIC_BASE + PLIC_CLAIM_BASE
                + ctx * PLIC_CTX_STRIDE, src);
}

#ifdef __cplusplus
}
#endif

#endif /* _ERBIUM_DRIVERS_PLIC_H_ */
