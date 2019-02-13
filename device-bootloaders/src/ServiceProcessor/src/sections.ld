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
    rom (rx)  : org = 0x40000000, l = 64K
    ram (!rx) : org = 0x40400000, l = 1M
}

SECTIONS
{
    /* . = 0x40400000; */
    . = 0x40000000; /* Loading into ROM for now, SP PC starts @ 0x40000000 */
    .text :
    {
        /* Place boot code first */
        *(.text.init)
        *(.text .text.* .gnu.linkonce.t.*)
    } > rom

    .rodata :
    {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    } > rom

    .data :
    {
        *(.data .data.* .gnu.linkonce.d.*)
    } > ram

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
        *(.sdata .sdata.* .gnu.linkonce.s.*)
    } > ram

    /* Align here to ensure crt.S bss init asm has an aligned start address */
    . = ALIGN(4);
    __bss_start = .;

    .bss :
    {
        *(.bss .bss.* .gnu.linkonce.b.*)
    } > ram

    .sbss :
    {
        *(.sbss .sbss.* .gnu.linkonce.sb.*)
    } > ram

    /* Align here to ensure crt.S bss init asm has an aligned end address */
    . = ALIGN(4);
    __bss_end = .;

    /* The default stack alignment is 16 bytes in RV32I and RV64I */
    . = ALIGN(16);

    stack :
    {
        __stack_base = .;
        *(stack stack.*)
    } > ram
}
