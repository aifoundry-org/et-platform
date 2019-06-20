.extern __bss_start
.extern __bss_end

#include "layout.h"

.section .text.init, "ax", @progbits
.global _start

_start:
    # initialize global pointer
.option push
.option norelax
    la gp, __global_pointer$
.option pop

    # set the stack pointer, different for each thread
    csrr  a0, mhartid
    li    a1, FW_MMODE_STACK_SIZE
    mul   a0, a0, a1
    addi  a0, a0, 8 # 1st stack element reserved as scratch region for traps, init sp to point ot it
    li    sp, FW_MMODE_STACK_BASE
    sub   sp, sp, a0

    # zero out the bss section
    la   x26, __bss_start
    la   x27, __bss_end
    bge  x26, x27, bss_loop_end
1:  sw   x0,  0(x26)
    addi x26, x26, 4
    blt  x26, x27, 1b

bss_loop_end:
    la   a1, main
    jalr x0, a1
