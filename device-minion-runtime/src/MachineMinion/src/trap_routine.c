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
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* minion_bl */
#include <etsoc/isa/riscv_encoding.h>
#include <etsoc/isa/hart.h>

extern int64_t syscall_handler(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3);

static void write_reg(uint64_t *const reg, uint64_t rd, uint64_t val);

uint64_t trap_routine(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t *const regs);

static inline uint64_t __attribute__((always_inline)) get_mhart_id(void)
{
    uint64_t ret;
    __asm__ __volatile__("csrr %0, mhartid" : "=r"(ret));
    return ret;
}

uint64_t trap_routine(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t *const regs)
{
    bool delegate = false;

    if (mcause == EXCEPTION_ILLEGAL_INSTRUCTION)
    {
        if ((mtval & INST_CSRRx_MASK) == INST_CSRRS_MHARTID)
        {
            uint64_t rd = (mtval >> 7) & 0x1F;
            write_reg(regs, rd, get_mhart_id());
        }
        else
        {
            /* Unhandled illegal instruction: delegate to S-mode */
            delegate = true;
        }
    }
    else
    { /* Unhandled exception */
        /* For now, delegate all of them to S-mode */
        delegate = true;
    }

    /* Delegate to S-mode by software */
    if (delegate)
    {
        uint64_t mstatus;
        uint64_t prev_mode;
        uint64_t stvec;

        /* Update S-mode exception info */
        asm volatile("csrw stval,  %0" : : "r"(mtval));
        asm volatile("csrw sepc,   %0" : : "r"(mepc));
        asm volatile("csrw scause, %0" : : "r"(mcause));

        asm volatile("csrr %0, mstatus" : "=r"(mstatus));

        prev_mode = (mstatus & MSTATUS_MPP) >> MSTATUS_MPP_SHIFT;

        /* Set MPP to S-mode */
        mstatus &= ~MSTATUS_MPP;
        mstatus |= (PRV_S << MSTATUS_MPP_SHIFT);

        /* Set SPP for S-mode: previous privilege is either U-mode or S-mode */
        mstatus &= ~MSTATUS_SPP;
        if (prev_mode == PRV_S)
            mstatus |= (1UL << MSTATUS_SPP_SHIFT);

        /* Set SPIE for S-mode */
        mstatus &= ~MSTATUS_SPIE;
        if (mstatus & MSTATUS_SIE)
            mstatus |= (1UL << MSTATUS_SPIE_SHIFT);

        /* Clear SIE for S-mode */
        mstatus &= ~MSTATUS_SIE;

        asm volatile("csrw mstatus, %0" : : "r"(mstatus));

        /* Return to S-mode exception vector base */
        asm volatile("csrr %0, stvec" : "=r"(stvec));

        return stvec;
    }

    // TODO: return +2 if compressed instruction ...
    return mepc + 4;
}

static void write_reg(uint64_t *const reg, uint64_t rd, uint64_t val)
{
    switch (rd)
    {
        case 0: // x0 has no effect
            break;
        case 2: // x2 is sp - write to mscratch to give them what they asked for
            asm volatile("csrw mscratch, %0" : : "r"(val));
            break;
        default:
            reg[rd] = val;
            break;
    }
}
