OUTPUT_ARCH("riscv")
ENTRY(_start)

BOOTROM_BASE = 0x0002008000;
BOOTROM_SIZE = 8K;
SRAM_BASE    = 0x000200C000;
SRAM_SIZE    = 4K;
MSYS_BASE    = 0x0040000000;
MSYS_SIZE    = 256;
MRAM_BASE    = 0x0040000100;
MRAM_SIZE    = 0xFFFF00;
STACK_SIZE   = DEFINED(__stack_size__) ? __stack_size__ : 1K;
MAX_THREADS  = 16;

MEMORY {
    BOOTROM  (rx)  : org = BOOTROM_BASE, len = BOOTROM_SIZE
    SRAM     (rwx) : org = SRAM_BASE,    len = SRAM_SIZE
    MSYS     (r)   : org = MSYS_BASE,    len = MSYS_SIZE
    MRAM     (rwx) : org = MRAM_BASE,    len = MRAM_SIZE
}

SECTIONS {
   .bootrom : {
        . = ALIGN(8);
        *(.bootrom .bootrom.*)
        *(.boot .boot.*)
    } > BOOTROM

    .text : {
        . = ALIGN(8);
        PROVIDE(__code_start = .);
        *(.text.init)
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(8);
        PROVIDE(__code_end = .);
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
        . = . + (MAX_THREADS * STACK_SIZE);
        PROVIDE(__stack_base = .);
    } > MRAM
}
