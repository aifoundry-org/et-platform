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

ENTRY(bl2_entry)

RAM_BASE_ADDR = 0x40400000;

RAM_SIZE = 896K; /* limit Service Processor BL2 RAM to 896 KB, because the last 128K is used by BL1 */

CODE_SIZE = 512K;
DATA_SIZE = RAM_SIZE - CODE_SIZE;

CODE_BASE_ADDR = RAM_BASE_ADDR;
DATA_BASE_ADDR = CODE_BASE_ADDR + CODE_SIZE;

STACK_SIZE = 0x4000;

MEMORY
{
    CODE (rx)  : org = CODE_BASE_ADDR, l = CODE_SIZE
    DATA (!rx) : org = DATA_BASE_ADDR, l = DATA_SIZE
}

SECTIONS
{
    .text : ALIGN(128)
    {
        /* Place boot code first */
        *(.text.init)
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
        /* ALIGN(128) above should ensure data init asm has 64-bit aligned start address */
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

        /* Align here to ensure data init asm has 64-bit aligned end address */
        . = ALIGN(8);
        __data_end = .;
    } > DATA

    .bss (NOLOAD) : ALIGN(128)
    {
        /* ALIGN(128) above should ensure bss init asm has a 64-bit aligned start address */
        __bss_start = .;

        *(.bss .bss.* .gnu.linkonce.b.*)
    } > DATA

    .sbss (NOLOAD) :
    {
        *(.sbss .sbss.* .gnu.linkonce.sb.*)

        /* Align here to ensure bss init asm has a 64-bit aligned end address */
        . = ALIGN(8);
        __bss_end = .;
    } > DATA

    /* The default stack alignment is 16 bytes in RV32I and RV64I */
    /* Used by main() before scheduler starts and ISRs, and by FreeRTOS afterwards */
    stack (NOLOAD) : ALIGN(16)
    {
        __stack_bottom = .;
        . += STACK_SIZE;
        __stack_top = .;
        __freertos_irq_stack_top = .;
        ASSERT(. == ALIGN(16), "stack must be 16-byte aligned");
    } > DATA

    heap (NOLOAD) : ALIGN(8)
    {
        __heap_start = .;
        __heap_end = RAM_BASE_ADDR + RAM_SIZE - .;
    } > DATA
}

