#include "esr_defines.h"
#include "riscv_encoding.h"

.section .text, "ax", @progbits /* TODO FIXME move to M +RX page sections */
.global trap_handler

.align 12 /* Minion requires machine trap vector to be 4K aligned, mtvec[11:0] = 0 */
.func
.option push
.option norelax
trap_handler:
    /* Get M-mode stack pointer */
    csrrw sp, mscratch, sp

    /* Allocate space for the context */
    addi  sp, sp, -(32 * 8)

    /* Save context (x1, x3) for fast-path exceptions */
    sd    x1,   1 * 8(sp)
    /*    x2    is sp, already saved to mscratch */
    sd    x3,   3 * 8(sp)

    /* Read exception cause. Interrupts have highest bit = 1 */
    csrr  x1, mcause
    bge   x1, x0, slow_path

    /* In slow-path we can only use x1 and x3 */
    andi  x1, x1, 0xFF
    li    x3, MACHINE_SOFTWARE_INTERRUPT
    /* For now we only handle Machine software interrupts (IPI) in fast-path */
    bne   x1, x3, slow_path

    /* Handle Machine software interrupt */
    csrr  x1, mhartid
    andi  x1, x1, 63
    li    x3, 1
    sll   x3, x3, x1
    li    x1, ESR_SHIRE(SHIRE_OWN, IPI_TRIGGER_CLEAR)
    sd    x3, 0(x1)
    fence
    /* Clear Machine software interrupt pending */
    csrci mip, 1 << MACHINE_SOFTWARE_INTERRUPT
    /* Set Supervisor software interrupt pending */
    csrsi mip, 1 << SUPERVISOR_SOFTWARE_INTERRUPT

    /* We are done, return */
    j     done_fast_path

    /* Slow-path: save test of the context and jump to C handler */
slow_path:
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

    /* Switch to the M-mode global pointer */
    la    gp, __global_pointer$

    /* Invoke trap routine with params (a0-a3, mcause, mpec, mtval, pointer to saved context) */
    csrr  a4, mcause
    csrr  a5, mepc
    csrr  a6, mtval
    mv    a7, sp
    jal   trap_routine

    /* Return value of trap routine is return address */
    csrw  mepc, a0

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
    /* Fast-path only saves x1 and x3 */
done_fast_path:
    ld    x3,   3 * 8(sp)
    /*    x2    is sp, restored later from mscratch */
    ld    x1,   1 * 8(sp)
    addi  sp, sp, (32 * 8)

    /* Restore stack pointer */
    csrrw sp, mscratch, sp

    mret
.option pop
.endfunc
