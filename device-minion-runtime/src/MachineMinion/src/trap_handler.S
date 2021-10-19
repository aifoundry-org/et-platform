/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

/* minion_rt_helpers */
#include "syscall_internal.h"

/* minion_bl */
#include <etsoc/isa/esr_defines.h>
#include <etsoc/isa/riscv_encoding.h>

.section .text, "ax", @progbits /* TODO FIXME move to M +RX page sections */

.macro esr_shire_addr dst, tmp, shire, prot, regno
    li    \dst, ESR_SHIRE_REGION | ((\prot) << ESR_REGION_PROT_SHIFT) | ((\regno) << 3)
    sll   \tmp, \shire, ESR_REGION_SHIRE_SHIFT
    or    \dst, \dst, \tmp
.endm

.macro esr_shire_this_prot_addr dst, prot, regno
    li    \dst, ESR_SHIRE_REGION | (SHIRE_OWN << ESR_REGION_SHIRE_SHIFT) | ((\regno) << 3)
    slli  \prot, \prot, ESR_REGION_PROT_SHIFT
    or    \dst, \dst, \prot
.endm

.macro esr_shire_ipi_trigger_addr dst, tmp, shire
    esr_shire_addr \dst, \tmp, \shire, ESR_SHIRE_IPI_TRIGGER_PROT, ESR_SHIRE_IPI_TRIGGER_REGNO
.endm

.align 12 /* Minion requires machine trap vector to be 4K aligned, mtvec[11:0] = 0 */
.func
.option push
.option norelax
.global trap_handler
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
    bge   x1, x0, handle_exception

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

handle_exception:
    /* Check for Environment call from S-mode (syscall) */
    li    x3, EXCEPTION_ENVIRONMENT_CALL_FROM_S_MODE
    bne   x1, x3, slow_path

    /* Register a0 carries syscall ID, a1-a3 args. Return value to a0 */
    /* For now we only handle some syscalls in fast-path. Regs a0, x1, x3 can be modified */
    li    x1, SYSCALL_IPI_TRIGGER_INT
    beq   a0, x1, syscall_ipi_trigger
    li    x1, SYSCALL_BROADCAST_INT
    bne   a0, x1, slow_path

    /* Handle SYSCALL_BROADCAST_INT */
    li    x1, ESR_SHIRE_PROT_ADDR(PRV_U, SHIRE_OWN, ESR_SHIRE_BROADCAST0)
    sd    a1, 0(x1)
    fence
    srli  x1, a3, ESR_BROADCAST_PROT_SHIFT
    andi  x1, x1, (ESR_BROADCAST_PROT_MASK >> ESR_BROADCAST_PROT_SHIFT)
    esr_shire_this_prot_addr x3, x1, (ESR_SHIRE_BROADCAST1 >> 3)
    li    x1, ESR_BROADCAST_ESR_SHIRE_MASK
    and   x1, a2, x1
    or    x1, x1, a3
    sd    x1, (x3)
    fence
    j     done_syscall_fast_path

syscall_ipi_trigger:
    /* Handle SYSCALL_IPI_TRIGGER_INT */
    esr_shire_ipi_trigger_addr x1, x3, a2
    sd    a1, 0(x1)

done_syscall_fast_path:
    /* Return to the following instruction that caused the trap */
    csrr  x1, mepc
    /* TODO: Support compressed instructions? */
    addi  x1, x1, 4
    csrw  mepc, x1

    /* We are done, return */
    j     done_fast_path

    /* Slow-path: save rest of the context and jump to C handler */
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

    /* Load x1 with cause. a0-a3 contains syscall args */
    csrr  x1, mcause

    /* In slow path, we directly handle syscall here instead of going to trap_routine */
    li    x3, EXCEPTION_ENVIRONMENT_CALL_FROM_S_MODE
    bne   x3, x1, some_other_exception  /* Not an environment call in slow path, so some other exception. */

    jal   syscall_handler               /* Environment call */
    sd    a0, 10 * 8(sp)                /* Update a0 in saved context with environment call return value */
    /* Return to the following instruction that caused the trap */
    csrr  x1, mepc
    /* TODO: Support compressed instructions? */
    addi  x1, x1, 4
    csrw  mepc, x1
    j     done_slow_path

some_other_exception:
    /* Invoke trap routine with params (a0-a3: mcause, mpec, mtval, pointer to saved context) */
    csrr  a0, mcause
    csrr  a1, mepc
    csrr  a2, mtval
    mv    a3, sp
    jal   trap_routine

    /* Return value of trap routine is return address */
    csrw  mepc, a0

done_slow_path:
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
