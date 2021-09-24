#include "etsoc/isa/riscv_encoding.h"

.section .text, "ax", @progbits /* TODO FIXME move to S +RX page sections */
.global trap_handler

.align 12 /* Minion requires machine trap vector to be 4K aligned, mtvec[11:0] = 0 */
.func

/* S-mode traps are not reentrant, but may come from S-mode or U-mode */
/* Note that interrupts are globally disabled, so if we receive an interrupt while in S-mode, it won't trap
   but just wake from WFI (if the local interrupt bits are set in sie) */
trap_handler:
    csrrw t0, sscratch, t0              /* Swap t0 with sscratch */
    sd    t1, 0 (t0)                    /* Save t1 in the S-mode stack scratch region */
    mv    t1, sp                        /* Set t1 to original sp */
    ld    sp, 8 (t0)                    /* Since we only trap from U-mode, switch to the S-mode stack */

    addi  sp, sp, -(32 * 8)             /* Make room for sp and saved context on stack */
    sd    t1, 2 * 8(sp)                 /* Save original sp (x2) */

    ld    t1, 0 (t0)                    /* Restore t1 from the S-mode stack scratch region */
    csrrw t0, sscratch, t0              /* Swap t0 with sscratch again */

    /* Save context */
    sd    x1,   1 * 8(sp)
    /* x2 is saved above */
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

    csrr  t0, scause                    /* Leave a0-a3 unmodified for environment calls */
    csrr  t1, sepc

    blt   t0, x0, handle_interrupt      /* Branch past exception handling if it's an interrupt. */

handle_exception:
    addi  t2, t1, 4                     /* Synchronous so update exception return address to the instruction after the instruction that generated the exception. */
    csrw sepc, t2                       /* Load sepc with the address of the next instruction in the task to run next. */

test_if_environment_call:
    li    t2, EXCEPTION_ENVIRONMENT_CALL_FROM_U_MODE
    bne   t2, t0, is_exception          /* Not an environment call, so some other exception. */
    mv    s1, sp                        /* Save SP here to print stack dump for critical calls */
    jal   syscall_handler               /* Environment call */
    sd    a0, 10 * 8(sp)                /* Update a0 in saved context with environment call return value */
    j     processed_source

handle_interrupt:
    li    t2, ((1 << 63) | SUPERVISOR_SOFTWARE_INTERRUPT)
    bne   t2, t0, as_yet_unhandled      /* Something as yet unhandled. */
    mv    a0, t0                        /* 1st argument = scause */
    mv    a1, t1                        /* 2nd argument = sepc */
    csrr  a2, stval                     /* 3rd argument = stval */
    mv    a3, sp                        /* 4th argument = pointer to regs on stack */
    la    ra, processed_source
    j     swi_handler                   /* Software interrupt */

is_exception:
    mv    a0, t0                        /* 1st argument = scause */
    mv    a1, t1                        /* 2nd argument = sepc */
    csrr  a2, stval                     /* 3rd argument = stval */
    mv    a3, sp                        /* 4th argument = pointer to regs on stack */
    la    ra, processed_source
    j     exception_handler             /* Handle exception */

as_yet_unhandled:
    ebreak                              /* Should never get here */
    j     as_yet_unhandled

processed_source:
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
    ld    x1,   1 * 8(sp)

    /* Stack pointer (x2) is the last register to be restored */
    ld    x2,   2 * 8(sp)

    sret
.endfunc
