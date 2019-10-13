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

    # R_SP_MISC region is 4K at 0x52029000, SP_BYPASS_CACHE is 32 bits at offset 0x14
    li   t0, 0x52029014
    lw   t1, 0(t0)
    # mask out all the bits except [0] Bypass ICache and [1] Bypass DCache
    and  t1, t1, 3
    # If the ICache and DCache are already enabled (bypass bits are cleared) we can skip to init_bss
    beqz t1, init_bss

    # before turning the L1 cache on, synchronize the instruction and data streams
    FENCE
    # Enable L1 I and D caches by clearing [0] Bypass ICache and [1] Bypass DCache
    sw   zero, 0(t0)
    # after turning the L1 cache on, synchronize the instruction and data streams
    FENCE

init_bss:
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
