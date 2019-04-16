.extern __bss_start
.extern __bss_end

#ifndef STACK_BASE
#define STACK_BASE 0x80001FFE00
#endif

#ifndef STACK_OFFSET
#define STACK_OFFSET 9 // 512 bytes
#endif

.section .text.init, "ax", @progbits
.global _start

_start:
    # set the stack pointer, different for each thread
    csrr  a0, mhartid
    slli  a0, a0, STACK_OFFSET
    li    sp, STACK_BASE
    sub   sp, sp, a0

    # zero out the bss section
    la   x26, __bss_start
    la   x27, __bss_end
    bge  x26, x27, bss_loop_end
1:  sw   x0,  0(x26)
    addi x26, x26, 4
    ble  x26, x27, 1b

bss_loop_end:

# initialize global pointer
.option push
.option norelax
    la gp, __global_pointer$
.option pop

    la   a1, main
    jalr x0, a1
