/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

/*
* Test: CacheOps on cacheable memory regions
*
* CacheOps are classified as writes in the Erbium PMA. BootROM is
* read-only, so all VA cache operations must set TensorError[7] there.
* On writable regions (MRAM, SRAM) they must succeed.
*/

#include "test.h"

#define MRAM_BASE       0x40000000ull
#define SRAM_BASE       0x0200C000ull
#define BOOTROM_BASE    0x0200A000ull

/* CSR addresses */
#define CSR_TENSOR_ERROR  0x808
#define CSR_EVICT_VA      0x89f
#define CSR_FLUSH_VA      0x8bf
#define CSR_PREFETCH_VA   0x81f

#define TENSOR_ERROR_PMA  (1 << 7)

/* EvictVA/FlushVA need dest >= 1 to reach the PMA check (dest=0 is L1-only, skipped) */
#define EVICT_VA(addr)    ((1ull << 58) | (addr))
#define FLUSH_VA(addr)    ((1ull << 58) | (addr))
#define PREFETCH_VA(addr) (addr)

static inline void clear_tensor_error(void) {
    asm volatile("csrw %0, zero" :: "i"(CSR_TENSOR_ERROR));
}

static inline uint64_t read_tensor_error(void) {
    uint64_t val;
    asm volatile("csrr %0, %1" : "=r"(val) : "i"(CSR_TENSOR_ERROR));
    return val;
}

/* Expect CacheOps to succeed (no PMA error) */
static void check_cacheops_ok(uint64_t addr) {
    clear_tensor_error();
    asm volatile("csrw %0, %1" :: "i"(CSR_EVICT_VA), "r"(EVICT_VA(addr)));
    if (read_tensor_error() & TENSOR_ERROR_PMA)
        TEST_FAIL;

    clear_tensor_error();
    asm volatile("csrw %0, %1" :: "i"(CSR_FLUSH_VA), "r"(FLUSH_VA(addr)));
    if (read_tensor_error() & TENSOR_ERROR_PMA)
        TEST_FAIL;

    clear_tensor_error();
    asm volatile("csrw %0, %1" :: "i"(CSR_PREFETCH_VA), "r"(PREFETCH_VA(addr)));
    if (read_tensor_error() & TENSOR_ERROR_PMA)
        TEST_FAIL;
}

/* Expect CacheOps to fail (PMA error) */
static void check_cacheops_fail(uint64_t addr) {
    clear_tensor_error();
    asm volatile("csrw %0, %1" :: "i"(CSR_EVICT_VA), "r"(EVICT_VA(addr)));
    if (!(read_tensor_error() & TENSOR_ERROR_PMA))
        TEST_FAIL;

    clear_tensor_error();
    asm volatile("csrw %0, %1" :: "i"(CSR_FLUSH_VA), "r"(FLUSH_VA(addr)));
    if (!(read_tensor_error() & TENSOR_ERROR_PMA))
        TEST_FAIL;

    clear_tensor_error();
    asm volatile("csrw %0, %1" :: "i"(CSR_PREFETCH_VA), "r"(PREFETCH_VA(addr)));
    if (!(read_tensor_error() & TENSOR_ERROR_PMA))
        TEST_FAIL;
}

int main() {
    /* MRAM: CacheOps should succeed */
    check_cacheops_ok(MRAM_BASE);

    /* SRAM: CacheOps should succeed */
    check_cacheops_ok(SRAM_BASE);

    /* BootROM: CacheOps must fail (read-only region) */
    check_cacheops_fail(BOOTROM_BASE);

    TEST_PASS;
    return 0;
}
