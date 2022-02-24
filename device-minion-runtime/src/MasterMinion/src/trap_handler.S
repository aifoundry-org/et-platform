#include <etsoc/isa/riscv_encoding.h>

.section .text, "ax", @progbits /* TODO FIXME move to S +RX page sections */

.align 12 /* Minion requires machine trap vector to be 4K aligned, mtvec[11:0] = 0 */

/* We only execute in S-mode in Master FW, and *never* in U-mode */
/* Note that software, external and timer should *never* trap as they are handled in the wfi main loop */
.func
.global trap_handler
trap_handler:
    /* Save t0 to sscratch */
    csrrw t0, sscratch, t0

    /* Allocate space for the context */
    addi  sp, sp, -(32 * 8)

    /* Restore t0 from sscratch */
    csrrw t0, sscratch, t0

    /* Save context */
    sd    x1,   1 * 8(sp)
    sd    x2,   2 * 8(sp)
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

    /* Switch to the S-mode global pointer */
.option push
.option norelax
    la    gp, __global_pointer$
.option pop

    /* Check interrupt or exception (Interrupts have highest bit = 1) */
    csrr  t0, scause
    blt   t0, x0, handle_interrupt      /* Branch past exception handling if it's an interrupt. */

    /* Invoke exception handler with params (scause, spec, stval, pointer to saved context) */
    csrr  a0, scause
    csrr  a1, sepc
    csrr  a2, stval
    mv    a3, sp
    jal   exception_handler

    /* Return value of exception handler is return address */
    csrw  sepc, a0
    j     processed_source

handle_interrupt:
    li    t1, ((1 << 63) | BUS_ERROR_INTERRUPT)  /* ET specific bus error interrupt */
    bne   t1, t0, as_yet_unhandled               /* Something as yet unhandled. */
    csrr  a0, scause                             /* 1st argument = scause */
    csrr  a1, sepc                               /* 2nd argument = sepc */
    csrr  a2, stval                              /* 3rd argument = stval */
    mv    a3, sp                                 /* 4th argument = pointer to regs on stack */
    la    ra, processed_source                   /* Set the return address */
    j     extended_interrupt                     /* ET specific interrupt handler */

processed_source:
    /* Restore context */
    ld    x1,   1 * 8(sp)
    /*    x2, is sp, restored later */
    ld    x3,   3 * 8(sp)
    ld    x4,   4 * 8(sp)
    ld    x5,   5 * 8(sp)
    ld    x6,   6 * 8(sp)
    ld    x7,   7 * 8(sp)
    ld    x8,   8 * 8(sp)
    ld    x9,   9 * 8(sp)
    ld    x10, 10 * 8(sp)
    ld    x11, 11 * 8(sp)
    ld    x12, 12 * 8(sp)
    ld    x13, 13 * 8(sp)
    ld    x14, 14 * 8(sp)
    ld    x15, 15 * 8(sp)
    ld    x16, 16 * 8(sp)
    ld    x17, 17 * 8(sp)
    ld    x18, 18 * 8(sp)
    ld    x19, 19 * 8(sp)
    ld    x20, 20 * 8(sp)
    ld    x21, 21 * 8(sp)
    ld    x22, 22 * 8(sp)
    ld    x23, 23 * 8(sp)
    ld    x24, 24 * 8(sp)
    ld    x25, 25 * 8(sp)
    ld    x26, 26 * 8(sp)
    ld    x27, 27 * 8(sp)
    ld    x28, 28 * 8(sp)
    ld    x29, 29 * 8(sp)
    ld    x30, 30 * 8(sp)

    addi  sp, sp, (32 * 8)

    sret

    /* Should never get here!! */
as_yet_unhandled:
    ebreak
    j     as_yet_unhandled

.endfunc
