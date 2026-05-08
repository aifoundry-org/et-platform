/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-----------------------------------------------------------------------*/

/* Generic register-access primitives — volatile load/store at a physical
 * address, read-modify-write, and poll-until-bits helpers. Not MMIO-
 * specific; ESR and other memory-mapped register paths use the same
 * primitives once the address is computed. */

#ifndef _COMMON_MMIO_H_
#define _COMMON_MMIO_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline __attribute__((always_inline))
uint32_t reg_read32(uintptr_t addr)
{
    return *(volatile const uint32_t *)addr;
}

static inline __attribute__((always_inline))
void reg_write32(uintptr_t addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
}

static inline __attribute__((always_inline))
void reg_modify32(uintptr_t addr, uint32_t mask, uint32_t val)
{
    uint32_t r = reg_read32(addr);

    reg_write32(addr, (r & ~mask) | (val & mask));
}

static inline __attribute__((always_inline))
uint64_t reg_read64(uintptr_t addr)
{
    return *(volatile const uint64_t *)addr;
}

static inline __attribute__((always_inline))
void reg_write64(uintptr_t addr, uint64_t val)
{
    *(volatile uint64_t *)addr = val;
}

/* Poll a register until (read & mask) == expected, or iteration limit
 * reached. Returns 0 on success, -1 on timeout. */
static inline __attribute__((always_inline))
int reg_wait_for_bits(uintptr_t addr, uint32_t mask, uint32_t expected,
                      uint32_t max_iter)
{
    while (max_iter--)
    {
        if ((reg_read32(addr) & mask) == expected)
        {
            return 0;
        }
    }
    return -1;
}

static inline __attribute__((always_inline))
int reg_wait_bit_set(uintptr_t addr, uint32_t bit, uint32_t max_iter)
{
    return reg_wait_for_bits(addr, bit, bit, max_iter);
}

static inline __attribute__((always_inline))
int reg_wait_bit_clear(uintptr_t addr, uint32_t bit, uint32_t max_iter)
{
    return reg_wait_for_bits(addr, bit, 0, max_iter);
}

#ifdef __cplusplus
}
#endif

#endif /* _COMMON_MMIO_H_ */
