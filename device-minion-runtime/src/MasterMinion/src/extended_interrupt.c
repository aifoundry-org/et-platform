/***********************************************************************
*
* Copyright (C) 2022 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
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
