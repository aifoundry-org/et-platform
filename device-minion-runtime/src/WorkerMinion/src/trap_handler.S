.section .text, "ax", @progbits /* TODO FIXME move to S +RX page sections */
.global trap_handler

.align 12 /* Minion requires machine trap vector to be 4K aligned, mtvec[11:0] = 0 */
.func

/* S-mode traps are not reentrant, but may come from S-mode or U-mode */
/* Note that interrupts are globally disabled, so if we receive an interrupt while in S-mode, it won't trap
   but (just wake from WFI if the local interrupt bits are set in sie) */
trap_handler:
    /* Get S-mode stack pointer */
    csrrw sp, sscratch, sp

    /* Allocate space for the context */
    addi  sp, sp, -(32 * 8)

    /* Save context */
    sd    x1,   1 * 8(sp)
    /* Read sscratch, which contains original sp */
    csrr  x1, sscratch
    sd    x1,   2 * 8(sp)
    sd    x3,   3 * 8(sp)
    sd    x4,   4 * 8(sp)
    sd    x5,   5 * 8(sp)
    sd    x6,   6 * 8(sp)
    sd    x7,   7 * 8(sp)
    sd    x8,   8 * 8(sp)
    sd    x9,   9 * 8(sp)
    sd    x10, 10 * 8(sp)
    sd    x11, 11 * 8(sp)
    sd    x12, 12 * 8(sp)
    sd    x13, 13 * 8(sp)
    sd    x14, 14 * 8(sp)
    sd    x15, 15 * 8(sp)
    sd    x16, 16 * 8(sp)
    sd    x17, 17 * 8(sp)
    sd    x18, 18 * 8(sp)
    sd    x19, 19 * 8(sp)
    sd    x20, 20 * 8(sp)
    sd    x21, 21 * 8(sp)
    sd    x22, 22 * 8(sp)
    sd    x23, 23 * 8(sp)
    sd    x24, 24 * 8(sp)
    sd    x25, 25 * 8(sp)
    sd    x26, 26 * 8(sp)
    sd    x27, 27 * 8(sp)
    sd    x28, 28 * 8(sp)
    sd    x29, 29 * 8(sp)
    sd    x30, 30 * 8(sp)
    sd    x31, 31 * 8(sp)

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
    sd    a0, 10 * 8(sp)                /* update a0 in saved context with environment call return value */
    j     processed_source

handle_asynchronous:
    lui   t1, 0x80000                   /* Asynchronous so set bit 31 */
    slli  t1, t1, 0x20                  /* Shift bit 31 to bit 63 */
    addi  t1, t1, 0x1                   /* 0x8000000000000001 == Supervisor software interrupt. */
    bne   t2, t1, as_yet_unhandled      /* Something as yet unhandled. */
    la    ra, processed_source
    j     swi_handler                   /* Software interrupt */

is_exception:
    mv    a0, t2                        /* 1st argument = scause */
    mv    a1, s1                        /* 2nd argument = sepc */
    csrr  a2, stval                     /* 3rd argument = stval */
    mv    a3, sp                        /* 4th argument = pointer to regs on stack */
    la    ra, processed_source
    j     exception_handler             /* Some unhandled synchronous exception */

as_yet_unhandled:
    ebreak                              /* Should never get here */
    j     as_yet_unhandled

processed_source:
    csrw sepc, s1                       /* Load sepc with the address of the next instruction in the task to run next. */

    /* Restore context */
    ld    x31, 31 * 8(sp)
    ld    x30, 30 * 8(sp)
    ld    x29, 29 * 8(sp)
    ld    x28, 28 * 8(sp)
    ld    x27, 27 * 8(sp)
    ld    x26, 26 * 8(sp)
    ld    x25, 25 * 8(sp)
    ld    x24, 24 * 8(sp)
    ld    x23, 23 * 8(sp)
    ld    x22, 22 * 8(sp)
    ld    x21, 21 * 8(sp)
    ld    x20, 20 * 8(sp)
    ld    x19, 19 * 8(sp)
    ld    x18, 18 * 8(sp)
    ld    x17, 17 * 8(sp)
    ld    x16, 16 * 8(sp)
    ld    x15, 15 * 8(sp)
    ld    x14, 14 * 8(sp)
    ld    x13, 13 * 8(sp)
    ld    x12, 12 * 8(sp)
    ld    x11, 11 * 8(sp)
    ld    x10, 10 * 8(sp)
    ld    x9,   9 * 8(sp)
    ld    x8,   8 * 8(sp)
    ld    x7,   7 * 8(sp)
    ld    x6,   6 * 8(sp)
    ld    x5,   5 * 8(sp)
    ld    x4,   4 * 8(sp)
    ld    x3,   3 * 8(sp)
    /*    x2    is sp, restored later from sscratch */
    ld    x1,   1 * 8(sp)

    addi  sp, sp, (32 * 8)

    /* Restore stack pointer */
    csrrw sp, sscratch, sp

    sret
.endfunc
