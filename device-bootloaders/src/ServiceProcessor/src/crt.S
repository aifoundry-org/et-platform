.extern __data_load_start
.extern __data_start
.extern __data_end
.extern __bss_start
.extern __bss_end
.extern __stack_base

#ifndef STACK_SIZE_LOG2
#define STACK_SIZE_LOG2 14
#endif

.section stack, "aw", @nobits
.space (1<<STACK_SIZE_LOG2),0

.section .text.init, "ax", @progbits
.global _start

_start:
    # set the stack pointer
    la   sp,  __stack_base

    # copy the data section into RAM
    la   x25, __data_load_start
    la   x26, __data_start
    la   x27, __data_end
    bge  x26, x27, data_loop_end
1:  lw   x24, 0(x25)
    addi x25, x25, 4
    sw   x24, 0(x26)
    addi x26, x26, 4
    blt  x26, x27, 1b

data_loop_end:
    # zero out the bss section
    la   x26, __bss_start
    la   x27, __bss_end
    bge  x26, x27, bss_loop_end
1:  sw   x0,  0(x26)
    addi x26, x26, 4
    blt  x26, x27, 1b

bss_loop_end:

# initialize global pointer
.option push
.option norelax
    la gp, __global_pointer$
.option pop

    la   a1,  main
    jalr x0,  a1
