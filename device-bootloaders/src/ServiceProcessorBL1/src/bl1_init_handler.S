.extern __bss_start
.extern __bss_end
.extern __stack_base

#ifndef STACK_SIZE_LOG2
#define STACK_SIZE_LOG2 12
#endif

.global _init_handler

_init_handler:
    # set the stack pointer
    la   sp,  __stack_base

    # zero out the bss section
    la   x26, __bss_start
    la   x27, __bss_end
    bge  x26, x27, bss_loop_end
1:  sw   x0,  0(x26)
    addi x26, x26, 4
    blt  x26, x27, 1b

bss_loop_end:

    la   a1,  bl1_main
    jalr x0,  a1

.section stack, "aw", @nobits
.space (1<<STACK_SIZE_LOG2),0
