/* Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Histogram demo for erbium / erbium-soc1sim.
 *
 * Naively parallel 256-bin histogram over a 256 x 256 grayscale image
 * supplied by the launcher. 16 harts partition the image row-wise
 * (16 rows / hart), build per-hart local histograms, and merge into
 * a global histogram using partition-owned writes — no atomics.
 *
 * Addressability:
 *   All buffers are carved out of region 0's heap, discovered at
 *   runtime via <erbium/isa/layout.h>'s __heap_regions[0]. The base
 *   of that region is a linker-resolved symbol that relocates with
 *   the rest of the kernel when the soc1sim runtime loader
 *   (mallocDevice) picks a dynamic DRAM address. The four buffers
 *   are anchored at the TOP of heap0 (i.e., computed backwards from
 *   heap0_end, which is always region_base + region_size — the
 *   "obvious" end of the allocated buffer), so the launcher can
 *   find them with just the device-buffer size and the fixed
 *   HIST_* offsets below.
 *
 * Launcher contract:
 *   1. Before launch — upload the 256 x 256 grayscale input image
 *      to offset (DEVICE_BUFFER_SIZE - HIST_LAYOUT_BYTES) of its
 *      16 MB device buffer. That is the first slot of our layout
 *      (IN), everything else sits after it.
 *   2. (Optional) dump device memory before launch.
 *   3. Launch the kernel.
 *   4. (Optional) dump device memory after launch.
 *   5. Read the summary at offset
 *        (DEVICE_BUFFER_SIZE - HIST_SUMMARY_OFFSET_FROM_END);
 *      check summary.magic == HIST_MAGIC and summary.sum_of_bins ==
 *      IMG_W * IMG_H. Then read the 1 KiB histogram at
 *        (DEVICE_BUFFER_SIZE - HIST_OUTPUT_OFFSET_FROM_END).
 *
 */

#include <stdint.h>
#include <stddef.h>

#include "erbium/isa/barriers.h"
#include "erbium/isa/cacheops-umode.h"
#include "erbium/isa/fcc.h"
#include "erbium/isa/hart.h"
#include "erbium/isa/utils.h"

/* Linker symbols, accessed via PC-relative `la` (auipc + addi) — no
 * absolute pointer table in .rodata. The soc1sim runtime loader
 * implicitly shifts every section by a load-delta; PC-relative
 * references between sections (code -> heap0) stay correct because
 * both ends shift by the same amount. We deliberately do NOT use
 * <erbium/isa/layout.h>'s __heap_regions here: that pointer-table
 * pattern needs R_RISCV_64 .rela.rodata entries, which today's
 * runtime ELF relocator chokes on. */
extern char heap0_start[];
extern char heap0_end[];

/* --------------------------------------------------------------- */
/* Topology-bound constants                                         */
/* --------------------------------------------------------------- */

#ifndef HIST_ITERATIONS
#define HIST_ITERATIONS 1
#endif

#define IMG_W          256u
#define IMG_H          256u

/* Only even-numbered harts (thread 0 of each minion) participate.
 * hart_id on soc1sim is minion*2 + thread, so hart & 1 == 0 picks
 * one hart per minion — i.e. no L1-sharing sibling pairs active at
 * the same time. If the L1 is configured in shared/split modes
 * where thread 0 and thread 1 alias the same cache, running both
 * threads simultaneously may reorder or drop writes in ways pure
 * 1-per-minion scheduling avoids. Keeps the algorithm identical;
 * just re-partitions. */
#define NUM_HARTS        8u                       /* active harts */
#define ROWS_PER_HART    (IMG_H / NUM_HARTS)      /* 32 */
#define STRIPE_BYTES     (ROWS_PER_HART * IMG_W)  /* 8192 */

#define BINS             256u
#define BINS_PER_HART    (BINS / NUM_HARTS)       /* 32 */

/* --------------------------------------------------------------- */
/* Buffer layout inside heap0 (anchored at the top, i.e. counted   */
/* backward from heap0_end). Offsets are kept as *_OFFSET_FROM_END */
/* so the launcher can compute them from DEVICE_BUFFER_SIZE alone. */
/* --------------------------------------------------------------- */

/* Every sub-region must start on a 64 B cache-line boundary so
 * cross-minion harts never share a line: L1/L2 are not coherent
 * across minions and simultaneous writes to the same line from
 * different minions race at writeback time. The per-hart slice of
 * LOCALS (NUM_HARTS × 1 KiB) and the per-hart slice of OUTPUT
 * (NUM_HARTS × 64 B) are already cache-line-sized, so aligning only
 * the region bases is sufficient. Summary is padded from 32 B to
 * 64 B for alignment. */
#define HIST_INPUT_BYTES      (IMG_W * IMG_H)            /* 64 KiB — 64 B aligned */
#define HIST_LOCALS_BYTES     (NUM_HARTS * BINS * 4u)    /* 16 KiB — 64 B aligned */
#define HIST_OUTPUT_BYTES     (BINS * 4u)                /*  1 KiB — 64 B aligned */
/* CSUMS: one 64 B cache line per hart — stripe byte-sum in the
 * first u32 of the line, rest pads for cache-line exclusivity.
 * We saw in multihart_test that putting 16 × u32 into a single
 * cache line drops 15 of 16 values on writeback; each hart must
 * own its own line. */
#define HIST_CSUMS_BYTES      (NUM_HARTS * 64u)          /*  1 KiB */
#define HIST_SUMMARY_BYTES    64u                        /* padded for alignment */

#define HIST_LAYOUT_BYTES \
    (HIST_INPUT_BYTES + HIST_LOCALS_BYTES + HIST_OUTPUT_BYTES + \
     HIST_CSUMS_BYTES + HIST_SUMMARY_BYTES)
/*  = 0x10000 + 0x4000 + 0x400 + 0x400 + 0x40 = 0x14840 */

/* Offsets from heap0_end (equivalently, from the very end of the
 * device buffer on soc1sim). heap0_end is 16 MiB-aligned, so all
 * of these offsets (each a multiple of 64) land on cache lines. */
#define HIST_INPUT_OFFSET_FROM_END \
    HIST_LAYOUT_BYTES
#define HIST_LOCALS_OFFSET_FROM_END \
    (HIST_LOCALS_BYTES + HIST_OUTPUT_BYTES + HIST_CSUMS_BYTES + HIST_SUMMARY_BYTES)
#define HIST_OUTPUT_OFFSET_FROM_END \
    (HIST_OUTPUT_BYTES + HIST_CSUMS_BYTES + HIST_SUMMARY_BYTES)
#define HIST_CSUMS_OFFSET_FROM_END \
    (HIST_CSUMS_BYTES + HIST_SUMMARY_BYTES)
#define HIST_SUMMARY_OFFSET_FROM_END  HIST_SUMMARY_BYTES

#define HIST_MAGIC         0xE0B10157u   /* "ERBIUM 0157 (HIST)" */

/* Summary is padded to a full cache line (64 B) so it never shares a
 * line with the tail of OUTPUT. Only hart 0 writes it, but keeping
 * it on its own line matches the rest of the layout's invariant. */
struct summary {
    uint32_t magic;
    uint32_t width;
    uint32_t height;
    uint32_t total_pixels;
    uint32_t sum_of_bins;   /* must equal total_pixels on success */
    uint32_t max_count;
    uint32_t max_index;
    uint32_t _reserved[9];  /* pad to 64 B (16 × u32) */
};

/* --------------------------------------------------------------- */
/* Shared synchronization                                           */
/* --------------------------------------------------------------- */

#define HIST_BARRIER_FLB       1u
#define HIST_BARRIER_MASK_T0   0xFFu
#define HIST_BARRIER_MASK_T1   0x00u

/* --------------------------------------------------------------- */
/* Kernel entry                                                     */
/* --------------------------------------------------------------- */

int main(void)
{
    const unsigned hart_id = get_hart_id();

    if (hart_id & 1u) {
        return 0;
    }
    /* Index among active harts: 0..7. All memory partitioning
     * (stripe, my_local slot, my_csum line, bin ownership) is
     * keyed off this, not the raw hart_id. */
    const unsigned hart = hart_id >> 1;

    /* Carve buffers out of the top of heap region 0. heap0_end is a
     * section-relative linker symbol that the runtime implicitly
     * shifts together with the rest of the kernel when it picks a
     * DRAM address, so a PC-relative `la` here yields the correct
     * runtime address without needing explicit ELF relocation. */
    uint8_t *const heap_end = (uint8_t *)heap0_end;

    uint8_t *const image =
        heap_end - HIST_INPUT_OFFSET_FROM_END;
    uint32_t *const locals = (uint32_t *)(
        heap_end - HIST_LOCALS_OFFSET_FROM_END);
    uint32_t *const global_h = (uint32_t *)(
        heap_end - HIST_OUTPUT_OFFSET_FROM_END);
    /* CSUMS region: one 64 B cache line per active hart (16 u32
     * slots; active hart H uses slot 0 of its line). */
    uint32_t *const csums = (uint32_t *)(
        heap_end - HIST_CSUMS_OFFSET_FROM_END);
    struct summary *const sum = (struct summary *)(
        heap_end - HIST_SUMMARY_OFFSET_FROM_END);

    /* Per-active-hart private slot (256 × u32 = 16 cache lines). */
    uint32_t *const my_local = locals + hart * BINS;
    const uint8_t *const my_stripe = image + hart * STRIPE_BYTES;
    /* Own our own cache line in CSUMS (stride 16 u32 = 64 B). */
    uint32_t *const my_csum = csums + hart * 16u;

    for (int _iter = 0; _iter < HIST_ITERATIONS; _iter++) {

    /* --------------------------------------------------------------
     * Phase 0 — local histogram over own stripe.
     * Each hart writes only its own 1 KiB slot of `locals`, so there
     * is no false sharing with neighbouring harts.
     * ------------------------------------------------------------ */
    for (unsigned i = 0; i < BINS; i++) my_local[i] = 0;
    /* Accumulate both the histogram and a stripe byte-sum checksum
     * in the SAME loop — single read per byte, so if the stripe
     * read returns a wrong value the checksum catches it with the
     * same magnitude as the histogram miscount. Diagnostic hook
     * for the intermittent ±1 sum-preserving bin-mismatch; the
     * launcher compares per-hart checksums against an independent
     * reference computed from input.bin host-side. */
    uint32_t stripe_csum = 0;
    for (size_t i = 0; i < STRIPE_BYTES; i++) {
        uint8_t v = my_stripe[i];
        my_local[v]++;
        stripe_csum += v;
    }
    my_csum[0] = stripe_csum;
    /* Order this-hart's accumulator stores before the cache op, then
     * push my_local out of L1 (and on to memory, matching how the
     * producer_consumer test-kernels do their cross-hart handoff)
     * so other harts see the committed counts in Phase 1. Without
     * this, the barrier only synchronizes control flow — L1 lines
     * holding my_local stay private to this hart, and the reader
     * hits its own cold L1 or a stale lower-level copy.
     *
     * my_csum is read-only by the launcher (no hart consumes it),
     * so we don't explicitly evict it — firmware's post-ecall L1
     * flush pushes it to memory before the dump. Keeping cacheop
     * pressure the same as the pre-diagnostic version so this
     * instrumentation itself doesn't perturb behaviour. */
    FENCE;
    evict(my_local, BINS * sizeof(uint32_t));
    WAIT_CACHEOPS;

    shire_barrier(HIST_BARRIER_FLB, FCC_0, NUM_HARTS,
                  HIST_BARRIER_MASK_T0, HIST_BARRIER_MASK_T1);

    /* --------------------------------------------------------------
     * Phase 1 — partition-owned global merge.
     * Each hart owns BINS_PER_HART = 16 bins, which is exactly one
     * 64-byte cache line of `global_h`. It reads the corresponding
     * slice of every local histogram (read-only cross-hart access,
     * safe between phase barriers) and writes its bins. No atomics,
     * no false sharing on the write.
     * ------------------------------------------------------------ */
    const unsigned bin_base = hart * BINS_PER_HART;
    for (unsigned b = 0; b < BINS_PER_HART; b++) {
        uint32_t acc = 0;
        for (unsigned h = 0; h < NUM_HARTS; h++) {
            acc += locals[h * BINS + bin_base + b];
        }
        global_h[bin_base + b] = acc;
    }
    /* Push this hart's slice of global_h (BINS_PER_HART × u32 = one
     * cache line) to memory so hart 0 sees a fresh copy in Phase 2. */
    FENCE;
    evict(&global_h[bin_base], BINS_PER_HART * sizeof(uint32_t));
    WAIT_CACHEOPS;

    shire_barrier(HIST_BARRIER_FLB, FCC_0, NUM_HARTS,
                  HIST_BARRIER_MASK_T0, HIST_BARRIER_MASK_T1);

    /* --------------------------------------------------------------
     * Phase 2 — hart 0 writes the summary.
     * ------------------------------------------------------------ */
    if (hart == 0) {
        uint32_t total = 0;
        uint32_t max_count = 0;
        uint32_t max_index = 0;
        for (unsigned i = 0; i < BINS; i++) {
            uint32_t c = global_h[i];
            total += c;
            if (c > max_count) {
                max_count = c;
                max_index = i;
            }
        }

        sum->magic        = HIST_MAGIC;
        sum->width        = IMG_W;
        sum->height       = IMG_H;
        sum->total_pixels = IMG_W * IMG_H;
        sum->sum_of_bins  = total;
        sum->max_count    = max_count;
        sum->max_index    = max_index;
        /* _reserved[] stays at its bss-zero value */
        FENCE;
    }

    } /* end HIST_ITERATIONS loop */

    return 0;
}
