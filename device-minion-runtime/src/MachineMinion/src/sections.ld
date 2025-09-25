/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

INCLUDE layout.ld

ENTRY(_start)

STACK_SIZE = 1024 + 64;
HART_COUNT = 2112;

MEMORY
{
    /* stacks are STACK_SIZE bytes per HART, HART_COUNT HARTs, descending from the end of M_MODE_REGION
       so ensure image fits in non-stack region */
    MCODE (rx)  : org = M_CODE_REGION_BASE, l = M_CODE_REGION_SIZE
    MDATA (!rx) : org = M_DATA_REGION_BASE, l = M_DATA_REGION_SIZE - (STACK_SIZE * HART_COUNT)
}

SECTIONS
{
    .text :
    {
        /* Starting at ESR_NEIGH_MINION_BOOT_RESET_VAL */
        . = 0x1000;
        *(.text.init)
        *(SORT_BY_ALIGNMENT(.text) SORT_BY_ALIGNMENT(.text.*) SORT_BY_ALIGNMENT(.gnu.linkonce.t.*))
    } > MCODE

    .rodata :
    {
        *(SORT_BY_ALIGNMENT(.rodata) SORT_BY_ALIGNMENT(.rodata.*) SORT_BY_ALIGNMENT(.gnu.linkonce.r.*))
        KEEP(*(SORT_BY_ALIGNMENT(.rodata.keep)))
    } > MCODE

    .rodata1 :
    {
        *(SORT_BY_ALIGNMENT(.rodata1))
    } > MCODE

    .sdata2 :
    {
        *(SORT_BY_ALIGNMENT(.sdata2) SORT_BY_ALIGNMENT(.sdata2.*) SORT_BY_ALIGNMENT(.gnu.linkonce.s2.*))
    } > MDATA

    .sbss2 :
    {
        *(SORT_BY_ALIGNMENT(.sbss2) SORT_BY_ALIGNMENT(.sbss2.*) SORT_BY_ALIGNMENT(.gnu.linkonce.sb2.*))
    } > MDATA

    .data :
    {
        *(.data .data.* .gnu.linkonce.d.*)
    } > MDATA

    /* We want the small data sections together, so single-instruction offsets
      can access them all, and initialized data all before uninitialized, so
      we can shorten the on-disk segment size. */
    .sdata :
    {
        /* the magic __global_pointer$ symbol is defined to point 0x800 bytes past the
          start of the .sdata section. The 0x800 magic number allows signed 12-bit
          offsets from __global_pointer$ to address symbols at the start of the .sdata
          section. The linker assumes that if this symbol is defined, then the gp
          register contains that value, which it can then use to relax accesses to
          global symbols within that 12-bit range.
          See https://www.sifive.com/blog/all-aboard-part-3-linker-relaxation-in-riscv-toolchain*/
        __global_pointer$ = . + 0x800;
        *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata .srodata.*)
        *(.sdata .sdata.* .gnu.linkonce.s.*)
    } > MDATA

    .bss (NOLOAD) :
    {
        *(.bss .bss.* .gnu.linkonce.b.*)
    } > MDATA

    .sbss (NOLOAD) :
    {
        *(.sbss .sbss.* .gnu.linkonce.sb.*)
    } > MDATA

    /DISCARD/ : { *(.riscv.attributes) }
}

ASSERT(SIZEOF(.bss) == 0, "Error, image is shared so .bss section must be empty");
ASSERT(SIZEOF(.sbss) == 0, "Error, image is shared so .sbss section must be empty");
