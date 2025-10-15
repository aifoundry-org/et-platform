/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file extended_interrupt.c
    \brief A C module that handles ET specific interrupts.

*/
/***********************************************************************/
/* mm_rt_svcs */
#include <etsoc/isa/macros.h>
#include <etsoc/isa/riscv_encoding.h>

/* mm specific headers */
#include "services/log.h"

void extended_interrupt(uint64_t scause, uint64_t sepc, uint64_t stval, const uint64_t *regs);

void extended_interrupt(uint64_t scause, uint64_t sepc, uint64_t stval, const uint64_t *regs)
{
    uint64_t sstatus;
    (void)regs;
    CSR_READ_SSTATUS(sstatus)

    /* Check if bus error interrupt */
    if ((scause & 0xFF) == BUS_ERROR_INTERRUPT)
    {
        /* Clear the bus error interrupt */
        asm volatile("csrc sip, %0" : : "r"(1 << BUS_ERROR_INTERRUPT));

        Log_Write(LOG_LEVEL_CRITICAL,
            "MM:Bus error interrupt:scause: %lx sepc: %lx stval: %lx sstatus: %lx\n", scause, sepc,
            stval, sstatus);
    }
    else
    {
        Log_Write(LOG_LEVEL_CRITICAL,
            "MM:Unhandled ET specific interrupt:scause: %lx sepc: %lx stval: %lx sstatus: %lx\n",
            scause, sepc, stval, sstatus);
    }
}
