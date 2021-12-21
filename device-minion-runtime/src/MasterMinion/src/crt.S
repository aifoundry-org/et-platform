#include <system/layout.h>

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
    li    sp, FW_SMODE_STACK_BASE
    sub   sp, sp, a0
    addi  sp, sp, -FW_SMODE_STACK_SCRATCH_REGION_SIZE # skip scratch region for C stack
    csrw  sscratch, sp                                # sscratch points to S-mode stack scratch region

    # we compile with -fno-zero-initialized-in-bss, therefore, no BSS is present

    call  main
