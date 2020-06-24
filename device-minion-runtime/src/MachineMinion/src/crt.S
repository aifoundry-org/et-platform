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
    li    sp, FW_MMODE_STACK_BASE
    sub   sp, sp, a0
    addi  sp, sp, -FW_MMODE_STACK_SCRATCH_REGION_SIZE # skip scratch region for C stack
    csrw  mscratch, sp                                # mscratch points to M-mode stack scratch region: 1st stack element reserved as scratch region for traps

    # we compile with -fno-zero-initialized-in-bss, therefore, no BSS is present

    la   a1, main
    jalr x0, a1
