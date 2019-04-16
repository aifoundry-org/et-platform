.section .text, "ax", @progbits /* TODO FIXME move to M +RX page sections */
.global trap_handler

.macro save_context
    addi sp, sp, -( 29 * 8 )
    sd x1,  1  * 8( sp )
    sd x3,  2  * 8( sp )
    sd x5,  3  * 8( sp )
    sd x6,  4  * 8( sp )
    sd x7,  5  * 8( sp )
    sd x8,  6  * 8( sp )
    sd x9,  7  * 8( sp )
    sd x10, 8  * 8( sp )
    sd x11, 9  * 8( sp )
    sd x12, 10 * 8( sp )
    sd x13, 11 * 8( sp )
    sd x14, 12 * 8( sp )
    sd x15, 13 * 8( sp )
    sd x16, 14 * 8( sp )
    sd x17, 15 * 8( sp )
    sd x18, 16 * 8( sp )
    sd x19, 17 * 8( sp )
    sd x20, 18 * 8( sp )
    sd x21, 19 * 8( sp )
    sd x22, 20 * 8( sp )
    sd x23, 21 * 8( sp )
    sd x24, 22 * 8( sp )
    sd x25, 23 * 8( sp )
    sd x26, 24 * 8( sp )
    sd x27, 25 * 8( sp )
    sd x28, 26 * 8( sp )
    sd x29, 27 * 8( sp )
    sd x30, 28 * 8( sp )
    sd x31, 29 * 8( sp )
    .endm

.macro restore_context
    ld  x1,  1  * 8( sp )
    ld  x3,  2  * 8( sp )
    ld  x5,  3  * 8( sp )
    ld  x6,  4  * 8( sp )
    ld  x7,  5  * 8( sp )
    ld  x8,  6  * 8( sp )
    ld  x9,  7  * 8( sp )
    ld  x10, 8  * 8( sp )
    ld  x11, 9  * 8( sp )
    ld  x12, 10 * 8( sp )
    ld  x13, 11 * 8( sp )
    ld  x14, 12 * 8( sp )
    ld  x15, 13 * 8( sp )
    ld  x16, 14 * 8( sp )
    ld  x17, 15 * 8( sp )
    ld  x18, 16 * 8( sp )
    ld  x19, 17 * 8( sp )
    ld  x20, 18 * 8( sp )
    ld  x21, 19 * 8( sp )
    ld  x22, 20 * 8( sp )
    ld  x23, 21 * 8( sp )
    ld  x24, 22 * 8( sp )
    ld  x25, 23 * 8( sp )
    ld  x26, 24 * 8( sp )
    ld  x27, 25 * 8( sp )
    ld  x28, 26 * 8( sp )
    ld  x29, 27 * 8( sp )
    ld  x30, 28 * 8( sp )
    ld  x31, 29 * 8( sp )
    addi sp, sp, 29 * 8
    .endm

.align 12 /* Minion requires machine trap vector to be 4K aligned, mtvec[11:0] = 0 */
.func
trap_handler:
    csrrw sp, mscratch, sp          /* Swap sp with mscratch TODO FIXME not reentrant */
    save_context

.option push
.option norelax
    la   gp, __global_pointer$      /* Switch to global pointer for this image */
.option pop

    csrr t2, mcause                 /* Leave a0-a3 unmodified for environment calls */
    csrr s1, mepc

    blt  t2, x0, 1f                 /* Branch past synchronous handling if not synchronous. */

    addi s1, s1, 4                  /* Synchronous so update exception return address to the instruction after the instruction that generated the exception. */
    li   t0, 0x8                    /* 0x8,9,B = environment call from U,S,M-mode. 0xA is reserved, so check bits 3:2. */
    andi t1, t2, 0xC
    bne  t0, t1, 2f                 /* Not an environment call, so some other exception. */
    jal  syscall_handler            /* Environment call */
    sd   a0, 8 * 8( sp )            /* update a0 in saved context with environment call return value */
    j    4f

1:  lui  t1, 0x80000                /* Asynchronous so set bit 31 */
    slli t1, t1, 0x20               /* Shift bit 31 to bit 63 */
    addi t1, t1, 0x3                /* 0x8000000000000003 == Machine software interrupt. */
    bne  t2, t1, 3f                 /* Something as yet unhandled. */
    la   ra, 4f
    j    swi_handler                /* Software interrupt */

2:  mv   a0, t2                     /* 1st argument = mcause */
    mv   a1, s1                     /* 2nd argument = mepc */
    csrr a2, mtval                  /* 3rd argument = mtval */
    mv   a3, sp                     /* 4th argument = pointer to regs on stack */
    la   ra, 4f
    j    exception_handler          /* Some unhandled synchronous exception */

3:  ebreak                          /* Some unhandled asynchronous exception, should never get here */
    j    4f

4:  csrw mepc, s1                   /* Load mepc with the address of the next instruction in the task to run next. */
    restore_context
    csrrw sp, mscratch, sp          /* Swap sp with mscratch */
    mret
    .endfunc
