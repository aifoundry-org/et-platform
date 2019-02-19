.extern __bss_start
.extern __bss_end

#ifndef STACK_BASE
#define STACK_BASE 0x8002000000 // ZeBu mini-SoC image only has 32MB of RAM total
#endif

#ifndef STACK_SIZE_LOG2
#define STACK_SIZE_LOG2 14
#endif

.section .text.init, "ax", @progbits
.global _start

_start:
    # **TODO FIXME loader should set the stack pointer**
    # set the stack pointer, different for each thread
    csrr  a1, mhartid
    slli  a1, a1, STACK_SIZE_LOG2
    li    sp, STACK_BASE
    sub   sp, sp, a1

    # **TODO FIXME loader should zero out the bss section**
    # zero out the bss section
    la   x26, __bss_start
    la   x27, __bss_end
    bge  x26, x27, bss_loop_end
1:  sw   x0,  0(x26)
    addi x26, x26, 4
    ble  x26, x27, 1b

bss_loop_end:
    la   a1, main
    jalr x0, a1
