#include "layout.h"

.section .text.init, "ax", @progbits
.global _start

_start:
    # initialize global pointer
.option push
.option norelax
    la gp, __global_pointer$
.option pop

    # Initialize the stack pointer for each thread
    csrr  t0, mhartid
    li    t1, FW_MMODE_STACK_SIZE
    mul   t0, t0, t1
    li    sp, FW_MMODE_STACK_BASE
    sub   sp, sp, t0

    # Set mscratch to the stack pointer
    csrw  mscratch, sp

    # we compile with -fno-zero-initialized-in-bss, therefore, no BSS is present

    # Jump to main()
    j     main
