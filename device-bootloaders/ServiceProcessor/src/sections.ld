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

ROM_BASE_ADDR = 0x40000000;
RAM_BASE_ADDR = 0x40400000;

ROM_SIZE = 64K;
RAM_SIZE = 1M;

MEMORY
{
    ROM (rx)  : org = ROM_BASE_ADDR, l = ROM_SIZE
    RAM (!rx) : org = RAM_BASE_ADDR, l = RAM_SIZE
}

SECTIONS
{
    .text :
    {
        /* Place boot code first */
        *(.text.init)
        *(SORT_BY_ALIGNMENT(.text) SORT_BY_ALIGNMENT(.text.*) SORT_BY_ALIGNMENT(.gnu.linkonce.t.*))
    } > ROM

    .rodata :
    {
        *(SORT_BY_ALIGNMENT(.rodata) SORT_BY_ALIGNMENT(.rodata.*) SORT_BY_ALIGNMENT(.gnu.linkonce.r.*))
        KEEP(*(SORT_BY_ALIGNMENT(.rodata.keep)))
    } > ROM

    .rodata1 :
    {
        *(SORT_BY_ALIGNMENT(.rodata1))
    } > ROM

    .sdata2 :
    {
        *(SORT_BY_ALIGNMENT(.sdata2) SORT_BY_ALIGNMENT(.sdata2.*) SORT_BY_ALIGNMENT(.gnu.linkonce.s2.*))
    } > ROM

    .sbss2 :
    {
        *(SORT_BY_ALIGNMENT(.sbss2) SORT_BY_ALIGNMENT(.sbss2.*) SORT_BY_ALIGNMENT(.gnu.linkonce.sb2.*))

    } > ROM

    .data :
    {
        /* Align here to ensure crt.S data init asm has aligned start end address */
        . = ALIGN(4);
        __data_start = .;

        *(SORT_BY_ALIGNMENT(.data) SORT_BY_ALIGNMENT(.data.*) SORT_BY_ALIGNMENT(.gnu.linkonce.d.*))
    } > RAM AT > ROM /* Copying data from ROM to RAM for now */

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

        /* Align here to ensure crt.S data init asm has aligned end address */
        . = ALIGN(4);
        __data_end = .;

    } > RAM AT > ROM

    .bss (NOLOAD) :
    {
        /* Align here to ensure crt.S bss init asm has an aligned start address */
        . = ALIGN(4);
        __bss_start = .;

        *(.bss .bss.* .gnu.linkonce.b.*)
    } > RAM

    .sbss (NOLOAD) :
    {
        *(.sbss .sbss.* .gnu.linkonce.sb.*)

        /* Align here to ensure crt.S bss init asm has an aligned end address */
        . = ALIGN(4);
        __bss_end = .;
    } > RAM

    /* Only used by main() before scheduler starts and ISRs */
    stack (NOLOAD) :
    {
        /* The default stack alignment is 16 bytes in RV32I and RV64I */
        . = ALIGN(16);

        KEEP(*(stack stack.*))
        __stack_base = .;
        __freertos_irq_stack_top = .;
        ASSERT(. == ALIGN(16), "stack must be 16-byte aligned");
    } > RAM

    . = ALIGN(4);

    heap (NOLOAD) :
    {
        __heap_start = .;
        __heap_end = RAM_BASE_ADDR + RAM_SIZE - .;
    } > RAM
}

__data_load_start = LOADADDR(.data);
