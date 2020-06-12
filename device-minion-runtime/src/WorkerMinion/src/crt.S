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
    csrr  a0, hartid
    li    a1, FW_SMODE_STACK_SIZE
    mul   a0, a0, a1
    addi  a0, a0, 16 # 1st stack element reserved for saved supervisor SP, 2nd element is scratch region for traps. Init sp to point to 2nd element
    li    sp, FW_SMODE_STACK_BASE
    sub   sp, sp, a0

    # we compile with -fno-zero-initialized-in-bss, therefore, no BSS is present

    la   a1, main
    jalr x0, a1
