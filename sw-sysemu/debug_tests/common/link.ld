OUTPUT_ARCH("riscv")
ENTRY(_start)

SP_ROM_BASE  = 0x0040000000;
SP_ROM_SIZE  = 128K;
SP_SRAM_BASE = 0x0040400000;
SP_SRAM_SIZE = 1M;
DRAM_BASE    = 0x8000001000;
DRAM_SIZE    = 4096M;
STACK_SIZE   = DEFINED(__stack_size__) ? __stack_size__ : 8K;
MAX_THREADS  = 2048;

MEMORY {
    SP_ROM  (rx)  : org = SP_ROM_BASE,  len = SP_ROM_SIZE
    SP_SRAM (rwx) : org = SP_SRAM_BASE, len = SP_SRAM_SIZE
    DRAM    (rwx) : org = DRAM_BASE,    len = DRAM_SIZE
}

SECTIONS {
    .text : {
        . = ALIGN(8);
        PROVIDE(__code_start = .);
        *(.text.init)
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(8);
        PROVIDE(__code_end = .);
    } > DRAM

    .data : {
        . = ALIGN(8);
        PROVIDE(__data_start = .);
        *(.data .data.*)
    } > DRAM

    .sdata : {
        __global_pointer$ = . + 0x800;
        . = ALIGN(8);
        *(.sdata .sdata.*)
        PROVIDE(__data_end = .);
    } > DRAM

    .bss (NOLOAD) : {
        . = ALIGN(8);
        PROVIDE(__bss_start = .);
        *(.bss .bss.*)
    } > DRAM

    .sbss (NOLOAD) : {
        *(.sbss .sbss.*)
        . = ALIGN(8);
        PROVIDE(__bss_end = .);
    } > DRAM

    .stack (NOLOAD) : {
        . = ALIGN(16);
        . = . + (MAX_THREADS * STACK_SIZE);
        PROVIDE(__stack_base = .);
    } > DRAM
}
