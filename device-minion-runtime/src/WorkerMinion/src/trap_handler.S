.section .text, "ax", @progbits /* TODO FIXME move to S +RX page sections */
.global trap_handler

.align 12 /* Minion requires machine trap vector to be 4K aligned, mtvec[11:0] = 0 */
.func

/* S-mode traps are not reentrant, but may come from S-mode or U-mode */
trap_handler:
    csrrw t0, sscratch, t0              /* Swap t0 with sscratch */
    sd    t1, 0 (t0)                    /* Save t1 in the S-mode stack scratch region */

    csrr  t1, sstatus                   /* Use t1 to check what privilege we trapped from */
    srli  t1, t1, 8
    andi  t1, t1, 1
    bne   t1, zero, from_s_mode         /* If we came from S-mode, skip switching the sp */

from_u_mode:
    mv    t1, sp                        /* Set t1 to original sp */
    ld    sp, 8 (t0)                    /* If we came from U-mode, switch to the S-mode stack */
    j     from_any_mode

from_s_mode:
    mv    t1, sp                        /* Set t1 to original sp */

from_any_mode:
    addi  sp, sp, -(30 * 8)             /* Make room for sp and saved context on stack */
    sd    t1, 29 * 8(sp)                /* Save original sp */

    ld    t1, 0 (t0)                    /* Restore t1 from the S-mode stack scratch region */
    csrrw t0, sscratch, t0              /* Swap t0 with sscratch again */

    sd    x1,  0  * 8( sp )             /* Save context */
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

.option push
.option norelax
    la    gp, __global_pointer$         /* switch to the S-mode global pointer */
.option pop

    csrr  t2, scause                    /* Leave a0-a3 unmodified for environment calls */
    csrr  s1, sepc

    blt   t2, x0, handle_asynchronous   /* Branch past synchronous handling if not synchronous. */

handle_synchronous:
    addi  s1, s1, 4                     /* Synchronous so update exception return address to the instruction after the instruction that generated the exception. */

test_if_environment_call:
    li    t0, 0x8                       /* 0x8 == user environment call */
    bne   t0, t2, is_exception          /* Not an environment call, so some other exception. */
    jal   syscall_handler               /* Environment call */
    sd    a0, 7 * 8( sp )               /* update a0 in saved context with environment call return value */
    j     processed_source

handle_asynchronous:
    lui   t1, 0x80000                   /* Asynchronous so set bit 31 */
    slli  t1, t1, 0x20                  /* Shift bit 31 to bit 63 */
    addi  t1, t1, 0x1                   /* 0x8000000000000001 == Supervisor software interrupt. */
    bne   t2, t1, as_yet_unhandled      /* Something as yet unhandled. */
    la    ra, processed_source
    j     swi_handler                   /* Software interrupt */

is_exception:
    la    ra, processed_source          /* Should never get here, no other exceptions are delegated to supervisor */
    j     exception_handler

as_yet_unhandled:
    ebreak                              /* Should never get here */
    j     as_yet_unhandled

processed_source:
    csrw sepc, s1                       /* Load sepc with the address of the next instruction in the task to run next. */

    ld    x1,  0  * 8( sp )             /* Restore context */
    ld    x3,  1  * 8( sp )
    ld    x5,  2  * 8( sp )
    ld    x6,  3  * 8( sp )
    ld    x7,  4  * 8( sp )
    ld    x8,  5  * 8( sp )
    ld    x9,  6  * 8( sp )
    ld    x10, 7  * 8( sp )
    ld    x11, 8  * 8( sp )
    ld    x12, 9  * 8( sp )
    ld    x13, 10 * 8( sp )
    ld    x14, 11 * 8( sp )
    ld    x15, 12 * 8( sp )
    ld    x16, 13 * 8( sp )
    ld    x17, 14 * 8( sp )
    ld    x18, 15 * 8( sp )
    ld    x19, 16 * 8( sp )
    ld    x20, 17 * 8( sp )
    ld    x21, 18 * 8( sp )
    ld    x22, 19 * 8( sp )
    ld    x23, 20 * 8( sp )
    ld    x24, 21 * 8( sp )
    ld    x25, 22 * 8( sp )
    ld    x26, 23 * 8( sp )
    ld    x27, 24 * 8( sp )
    ld    x28, 25 * 8( sp )
    ld    x29, 26 * 8( sp )
    ld    x30, 27 * 8( sp )
    ld    x31, 28 * 8( sp )

    ld    sp,  29 * 8( sp )             /* Restore original sp */

    sret
    .endfunc
