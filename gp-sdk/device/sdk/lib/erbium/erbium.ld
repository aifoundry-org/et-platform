/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

OUTPUT_ARCH("riscv")
ENTRY(_boot)

/* Erbium memory layout.
 *
 *   MRAM_BASE
 *   ├── [0 .. MBOOT_OFFSET)       reserved for runtime-supplied config
 *   ├── .mboot                    M-mode first-stage (boot.S), runs
 *   │                             before dropping into U-mode
 *   ├── [MMODE_RESERVED_SIZE]     end of M-mode reserved window
 *   ├── .text.init / .text        U-mode kernel code
 *   ├── .rodata / .data / .sdata
 *   ├── .bss   / .sbss
 *   ├── stack  NUM_HARTS * STACK_SIZE (descending; __stack_base = top)
 *   └── heap   rest of MRAM, exposed via __heap_regions[]
 *
 * All knobs below are `PROVIDE`'d so they can be overridden at the
 * link line, e.g. -Wl,--defsym=MBOOT_OFFSET=0x100.
 */

/* TODO: halification - use constants from erbium-hal */
MRAM_BASE = 0x0040000000;
MRAM_SIZE = 16M;

PROVIDE(MMODE_RESERVED_SIZE = 4K);
PROVIDE(MBOOT_OFFSET        = 64);  /* config region size at MRAM start */
PROVIDE(NUM_HARTS           = 16);
PROVIDE(STACK_SIZE          = 1024);

MEMORY {
    /* M-mode reserved window (first MMODE_RESERVED_SIZE of MRAM). */
    MMODE (rx)  : org = MRAM_BASE,
                  len = MMODE_RESERVED_SIZE

    /* U-mode code/data/heap — MRAM minus the reserved window. */
    MRAM  (rwx) : org = MRAM_BASE + MMODE_RESERVED_SIZE,
                  len = MRAM_SIZE - MMODE_RESERVED_SIZE
}

SECTIONS {
    /* M-mode first stage lives at MRAM_BASE + MBOOT_OFFSET so the
     * first MBOOT_OFFSET bytes stay free for the uploader's config
     * payload. */
    .mboot MRAM_BASE + MBOOT_OFFSET : {
        *(.mboot .mboot.*)
    } > MMODE

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
        . = ALIGN(16);
        PROVIDE(__stack_start = .);
        . = . + (NUM_HARTS * STACK_SIZE);
        . = ALIGN(16);
        PROVIDE(__stack_base = .);
    } > MRAM

    /* Everything from __heap_start to end-of-MRAM is available to
     * user-mode allocators via <erbium/isa/layout.h>. */
    .heap (NOLOAD) : {
        . = ALIGN(16);
        PROVIDE(__heap_start = .);
        PROVIDE(heap0_start = .);
        . = ORIGIN(MRAM) + LENGTH(MRAM);
        PROVIDE(__heap_end = .);
        PROVIDE(heap0_end = .);
    } > MRAM

    __heap_size = __heap_end - __heap_start;
    heap0_size = heap0_end - heap0_start;

    /DISCARD/ : { *(.riscv.attributes) }
}

ASSERT(__heap_start <= __heap_end, "Heap start is past end of MRAM; reduce code/stack budget")
