/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

/*
 * erbium-soc1sim kernel layout.
 *
 * Up to 8 SoC memory regions (buffers) are injected at link time via
 *   -Wl,--defsym=regionN_base=<addr>,--defsym=regionN_size=<bytes>
 * Region 0 is special: it hosts the kernel's code + stack; whatever
 * space is left in region 0 after code/stack becomes heap0.
 *
 * Regions 1..7 currently only contribute linker-level aliases
 * (heapN_start / heapN_size) for future use; the library's
 * __heap_regions[] array only exposes heap0 until the runtime
 * loader supports post-load "live patching" of the multi-region
 * table. See src/erbium-soc1sim/erbium/isa/layout.c.
 *
 * A no-defsym build still links — region 0 defaults match native
 * erbium so the examples keep building.
 */

OUTPUT_ARCH("riscv")
ENTRY(_boot)

/* Raw SoC region descriptors — override with --defsym at link time.
 *
 * region0_base defaults to the etsoc1 DRAM base (0x8005801000),
 * matching test-compute-kernels. The soc1sim runtime's ELF
 * relocator currently cannot handle any .rela* section without
 * crashing, so we rely on the runtime placing our kernel at its
 * link-time VMA. Tracking this as a runtime bug; once fixed, the
 * kernel will work at any dynamically-chosen base. */
PROVIDE(region0_base = 0x8005801000);
PROVIDE(region0_size = 0x01000000);
PROVIDE(region1_base = 0);  PROVIDE(region1_size = 0);
PROVIDE(region2_base = 0);  PROVIDE(region2_size = 0);
PROVIDE(region3_base = 0);  PROVIDE(region3_size = 0);
PROVIDE(region4_base = 0);  PROVIDE(region4_size = 0);
PROVIDE(region5_base = 0);  PROVIDE(region5_size = 0);
PROVIDE(region6_base = 0);  PROVIDE(region6_size = 0);
PROVIDE(region7_base = 0);  PROVIDE(region7_size = 0);

/* Configurable layout knobs. The etsoc1 runtime requires the
 * *total* stack region to be a multiple of 4 KiB and 4 KiB aligned;
 * that's a runtime bookkeeping constraint, not a hardware one.
 * Link-time asserts near the end of this file enforce this. */
PROVIDE(MMODE_RESERVED_SIZE = 4K);
PROVIDE(NUM_HARTS           = 16);
PROVIDE(STACK_SIZE          = 1024);  /* per hart; override with -Wl,--defsym=STACK_SIZE=<bytes> */

/* MEMORY block binds all our sections into one named region so that
 * `> MRAM` is meaningful and `. = ORIGIN(MRAM) + LENGTH(MRAM)` works
 * as an absolute assignment inside output-section bodies. Without
 * the binding, `.` inside a section body behaves as a section-start
 * offset rather than an absolute address. */
MEMORY {
    MRAM (rwx) : ORIGIN = region0_base + MMODE_RESERVED_SIZE,
                 LENGTH = region0_size - MMODE_RESERVED_SIZE
}

SECTIONS {
    .text : {
        . = ALIGN(8);
        PROVIDE(__code_start = .);
        *(.text.init)
        *(.text .text.*)
        . = ALIGN(8);
        PROVIDE(__code_end = .);
    } > MRAM

    .rodata : {
        *(SORT_BY_ALIGNMENT(.rodata) SORT_BY_ALIGNMENT(.rodata.*) SORT_BY_ALIGNMENT(.gnu.linkonce.r.*))
    } > MRAM

    .data : {
        . = ALIGN(8);
        PROVIDE(__data_start = .);
        *(.data .data.*)
    } > MRAM

    .sdata : {
        __global_pointer$ = . + 0x800;
        . = ALIGN(8);
        *(.sdata .sdata.*)
        PROVIDE(__data_end = .);
    } > MRAM

    .bss (NOLOAD) : {
        . = ALIGN(8);
        PROVIDE(__bss_start = .);
        *(.bss .bss.*)
    } > MRAM

    .sbss (NOLOAD) : {
        *(.sbss .sbss.*)
        . = ALIGN(8);
        PROVIDE(__bss_end = .);
    } > MRAM

    .stack (NOLOAD) : {
        . = ALIGN(4K);
        PROVIDE(__stack_start = .);
        . = . + (NUM_HARTS * STACK_SIZE);
        PROVIDE(__stack_base = .);
    } > MRAM

    /* heap0 = whatever's left in region 0 after code+stack.
     *
     * Placed inside its own NOLOAD output section bound to MRAM so
     * both heap0_start and heap0_end are section-relative symbols —
     * a runtime ELF relocator that moves the kernel shifts them
     * together and the heap_region_t entry in .rodata stays
     * consistent. */
    .heap0 (NOLOAD) : {
        . = ALIGN(16);
        heap0_start = .;
        . = ORIGIN(MRAM) + LENGTH(MRAM);
        heap0_end = .;
    } > MRAM

    heap0_size = heap0_end - heap0_start;

    /* heap1..heap7 are the raw region buffers verbatim, aliased for
     * future live-patching by the runtime loader. Not currently
     * exposed through __heap_regions[]. */
    heap1_start = region1_base;  heap1_size = region1_size;
    heap2_start = region2_base;  heap2_size = region2_size;
    heap3_start = region3_base;  heap3_size = region3_size;
    heap4_start = region4_base;  heap4_size = region4_size;
    heap5_start = region5_base;  heap5_size = region5_size;
    heap6_start = region6_base;  heap6_size = region6_size;
    heap7_start = region7_base;  heap7_size = region7_size;

    /DISCARD/ : { *(.riscv.attributes) }
}

ASSERT(heap0_start <= heap0_end,
       "code+stack overflows region 0 — grow region0_size or shrink kernel/stack")

/* etsoc1 runtime stack-region contract: total size is a multiple of
 * 4 KiB and the region starts on a 4 KiB boundary. */
ASSERT(((NUM_HARTS * STACK_SIZE) & 0xFFF) == 0,
       "etsoc1 runtime requires TOTAL stack size (NUM_HARTS * STACK_SIZE) to be a multiple of 4 KiB")
ASSERT((__stack_start & 0xFFF) == 0,
       "etsoc1 runtime requires __stack_start to be 4 KiB aligned")
