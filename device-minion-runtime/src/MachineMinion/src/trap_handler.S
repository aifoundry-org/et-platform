.section .text, "ax", @progbits /* TODO FIXME move to M +RX page sections */
.global trap_handler

.align 12 /* Minion requires machine trap vector to be 4K aligned, mtvec[11:0] = 0 */
.func
trap_handler:
    csrrw t0, mscratch, t0          /* Swap t0 with mscratch */
    sd    t1, 0 (t0)                /* Save t1 in the M-mode stack scratch region */

    csrr  t1, mstatus               /* Use t1 to check what privilege we trapped from */
    srli  t1, t1, 11
    andi  t1, t1, 3
    xori  t1, t1, 3
    beq   t1, zero, from_m_mode     /* If we came from M-mode, skip switching the sp */

    mv    t1, sp                    /* Set t1 to original sp */
    mv    sp, t0                    /* If we came from U or S-mode, switch to the M-mode stack */

.option push
.option norelax
    la    gp, __global_pointer$     /* If we came from U or S-mode, switch to the M-mode global pointer */
.option pop

    j     from_any_mode

from_m_mode:
    mv    t1, sp                    /* Set t1 to original sp */

from_any_mode:
    addi  sp, sp, -(32 * 8)         /* Make room for sp, mepc, mstatus and saved context on stack */
    sd    t1, 31 * 8(sp)            /* Save original sp */

    ld    t1, 0 (t0)                /* Restore t1 from the M-mode stack scratch region */
    csrrw t0, mscratch, t0          /* Swap t0 with mscratch again */

    sd    x1,  0  * 8( sp )         /* Save context */
    sd    x3,  1  * 8( sp )
    sd    x5,  2  * 8( sp )
    sd    x6,  3  * 8( sp )
    sd    x7,  4  * 8( sp )
    sd    x8,  5  * 8( sp )
    sd    x9,  6  * 8( sp )
    sd    x10, 7  * 8( sp )
    sd    x11, 8  * 8( sp )
    sd    x12, 9  * 8( sp )
    sd    x13, 10 * 8( sp )
    sd    x14, 11 * 8( sp )
    sd    x15, 12 * 8( sp )
    sd    x16, 13 * 8( sp )
    sd    x17, 14 * 8( sp )
    sd    x18, 15 * 8( sp )
    sd    x19, 16 * 8( sp )
    sd    x20, 17 * 8( sp )
    sd    x21, 18 * 8( sp )
    sd    x22, 19 * 8( sp )
    sd    x23, 20 * 8( sp )
    sd    x24, 21 * 8( sp )
    sd    x25, 22 * 8( sp )
    sd    x26, 23 * 8( sp )
    sd    x27, 24 * 8( sp )
    sd    x28, 25 * 8( sp )
    sd    x29, 26 * 8( sp )
    sd    x30, 27 * 8( sp )
    sd    x31, 28 * 8( sp )
    csrr  t0, mstatus
    sd    t0,  29 * 8( sp )          /* Save mstatus */

    csrr  t1, mepc
    csrr  t2, mcause                /* Leave a0-a3 unmodified for environment calls */

    blt   t2, zero, 1f              /* Branch past synchronous handling if not synchronous. */

    addi  t1, t1, 4                 /* Synchronous so update exception return address to the instruction after the instruction that generated the exception. */
    sd    t1, 30 * 8(sp)            /* Save mepc on stack */
    li    t0, 0x8                   /* 0x8,9,B = environment call from U,S,M-mode. 0xA is reserved, so check bits 3:2. */
    andi  t3, t2, 0xC
    bne   t0, t3, 2f                /* Not an environment call, so some other exception. */
    jal   syscall_handler           /* Environment call */
    sd    a0, 7 * 8( sp )           /* update a0 in saved context with environment call return value */
    j     4f

1:  sd    t1, 30 * 8(sp)            /* Save mepc on stack */
    lui   t3, 0x80000               /* Asynchronous so set bit 31 */
    slli  t3, t3, 0x20              /* Shift bit 31 to bit 63 */
    addi  t3, t3, 0x3               /* 0x8000000000000003 == Machine software interrupt. */
    bne   t2, t3, 3f                /* Something as yet unhandled. */
    la    ra, 4f
    j     swi_handler               /* Software interrupt */

2:  mv    a0, t2                    /* 1st argument = mcause */
    mv    a1, t1                    /* 2nd argument = mepc */
    csrr  a2, mtval                 /* 3rd argument = mtval */
    mv    a3, sp                    /* 4th argument = pointer to regs on stack */
    la    ra, 4f
    j     exception_handler         /* Some unhandled synchronous exception */

3:  ebreak                          /* Some unhandled asynchronous exception, should never get here */
    j     4f

4:  ld   x1,  0  * 8( sp )          /* Restore context (except x31) */
    ld   x3,  1  * 8( sp )
    ld   x5,  2  * 8( sp )
    ld   x6,  3  * 8( sp )
    ld   x7,  4  * 8( sp )
    ld   x8,  5  * 8( sp )
    ld   x9,  6  * 8( sp )
    ld   x10, 7  * 8( sp )
    ld   x11, 8  * 8( sp )
    ld   x12, 9  * 8( sp )
    ld   x13, 10 * 8( sp )
    ld   x14, 11 * 8( sp )
    ld   x15, 12 * 8( sp )
    ld   x16, 13 * 8( sp )
    ld   x17, 14 * 8( sp )
    ld   x18, 15 * 8( sp )
    ld   x19, 16 * 8( sp )
    ld   x20, 17 * 8( sp )
    ld   x21, 18 * 8( sp )
    ld   x22, 19 * 8( sp )
    ld   x23, 20 * 8( sp )
    ld   x24, 21 * 8( sp )
    ld   x25, 22 * 8( sp )
    ld   x26, 23 * 8( sp )
    ld   x27, 24 * 8( sp )
    ld   x28, 25 * 8( sp )
    ld   x29, 26 * 8( sp )
    ld   x30, 27 * 8( sp )

    ld   x31, 29 * 8( sp )           /* Restore mstatus */
    csrw mstatus, x31
    ld   x31, 30 * 8( sp )           /* Restore mepc */
    csrw mepc, x31
    ld   x31, 28 * 8( sp )           /* Restore x31 */
    ld   sp,  31 * 8( sp )           /* Restore original sp */

    mret
    .endfunc
