/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

ENTRY(_boot)

MEMORY
{
    /* Mcode region is 2M from 0x8000000000 to 0x8000200000
       stacks are 768B per HART, 2112 HARTs, descending from @0x80001FFD00
       so ensure image fits in non-stack region */
    MCODE (rwx) : org = 0x8000000000, l = 0x73D00
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
    } > MCODE

    .rodata1 :
    {
        *(SORT_BY_ALIGNMENT(.rodata1))
    } > MCODE

    .sdata2 :
    {
        *(SORT_BY_ALIGNMENT(.sdata2) SORT_BY_ALIGNMENT(.sdata2.*) SORT_BY_ALIGNMENT(.gnu.linkonce.s2.*))
    } > MCODE

    .sbss2 :
    {
        *(SORT_BY_ALIGNMENT(.sbss2) SORT_BY_ALIGNMENT(.sbss2.*) SORT_BY_ALIGNMENT(.gnu.linkonce.sb2.*))
    } > MCODE

    .data :
    {
        *(.data .data.* .gnu.linkonce.d.*)
    } > MCODE

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
    } > MCODE

    .bss :
    {
        /* Align here to ensure crt.S bss init asm has an aligned start address */
        . = ALIGN(4);
        __bss_start = .;

        *(.bss .bss.* .gnu.linkonce.b.*)
    } > MCODE

    .sbss :
    {
        *(.sbss .sbss.* .gnu.linkonce.sb.*)

        /* Align here to ensure crt.S bss init asm has an aligned end address */
        . = ALIGN(4);
        __bss_end = .;
    } > MCODE
}
