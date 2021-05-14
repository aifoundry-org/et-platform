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

ENTRY(bl1_entry)

CODE_BASE_ADDR = 0x404E0000;
DATA_BASE_ADDR = 0x404F0000;

CODE_SIZE = 64K; /* limit Service Processor BL1 code to 64 KB */
DATA_SIZE = 64K; /* limit Service Processor BL1 zero-initialized data and stack to 64 KB */

MEMORY
{
    CODE (rx)  : org = CODE_BASE_ADDR, l = CODE_SIZE
    DATA (!rx) : org = DATA_BASE_ADDR, l = DATA_SIZE
}

SECTIONS
{
    .text : ALIGN(128)
    {
        *(SORT_BY_ALIGNMENT(.text) SORT_BY_ALIGNMENT(.text.*) SORT_BY_ALIGNMENT(.gnu.linkonce.t.*))
    } > CODE

    .rodata :
    {
        *(SORT_BY_ALIGNMENT(.rodata) SORT_BY_ALIGNMENT(.rodata.*) SORT_BY_ALIGNMENT(.gnu.linkonce.r.*))
    } > CODE

    .rodata1 :
    {
        *(SORT_BY_ALIGNMENT(.rodata1))
    } > CODE

    .sdata2 :
    {
        *(SORT_BY_ALIGNMENT(.sdata2) SORT_BY_ALIGNMENT(.sdata2.*) SORT_BY_ALIGNMENT(.gnu.linkonce.s2.*))
    } > CODE

    .sbss2 :
    {
        *(SORT_BY_ALIGNMENT(.sbss2) SORT_BY_ALIGNMENT(.sbss2.*) SORT_BY_ALIGNMENT(.gnu.linkonce.sb2.*))
    } > CODE

    .data : ALIGN(128)
    {
        /* ALIGN(128) above should ensure crt.S data init asm has 64-bit aligned start address */
        __data_start = .;

        *(SORT_BY_ALIGNMENT(.data) SORT_BY_ALIGNMENT(.data.*) SORT_BY_ALIGNMENT(.gnu.linkonce.d.*))
    } > DATA

    /* We want the small data sections together, so single-instruction offsets
      can access them all, and initialized data all before uninitialized, so
      we can shorten the on-disk segment size.  */
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
        *(SORT_BY_ALIGNMENT(.sdata) SORT_BY_ALIGNMENT(.sdata.*) SORT_BY_ALIGNMENT(.gnu.linkonce.s.*))

        /* Align here to ensure crt.S data init asm has 64-bit aligned end address */
        . = ALIGN(8);
        __data_end = .;
    } > DATA

    .bss (NOLOAD) : ALIGN(128)
    {
        /* ALIGN(128) above should ensure crt.S bss init asm has a 64-bit aligned start address */
        __bss_start = .;

        *(.bss .bss.* .gnu.linkonce.b.*)
    } > DATA

    .sbss (NOLOAD) :
    {
        *(.sbss .sbss.* .gnu.linkonce.sb.*)

        /* Align here to ensure crt.S bss init asm has a 64-bit aligned end address */
        . = ALIGN(8);
        __bss_end = .;
    } > DATA

    /* The default stack alignment is 16 bytes in RV32I and RV64I */
    . = ALIGN(16);

    stack (NOLOAD) :
    {
        KEEP(*(stack stack.*))
        __stack_base = .;
    } > DATA
}
