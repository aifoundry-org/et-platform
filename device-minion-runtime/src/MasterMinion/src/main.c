/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file consists the main method that serves as the entry
*       point for the the c runtime.
*
*   FUNCTIONS
*
*       main
*
***********************************************************************/
/* mm_rt_svcs */
#include <etsoc/common/common_defs.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/atomic.h>
#include <etsoc/isa/riscv_encoding.h>

/* mm specific headers */
#include "dispatcher/dispatcher.h"
#include "workers/spw.h"
#include "workers/sqw.h"
#include "workers/sqw_hp.h"
#include "workers/kw.h"
#include "workers/dmaw.h"
#include "services/log.h"

/*! \def EVEN_HART(x)
    \brief Macro to check the even or odd parity of the hart
*/
#define EVEN_HART(x) (x % 2 == 0)

/*! \var spinlock_t Launch_Wait
    \brief Spinlock used to let the FW workers continue.
    Released by the Dispatcher.
    \warning Not thread safe!
*/
spinlock_t Launch_Wait = { 0 };

void main(void);

void main(void)
{
    uint64_t temp;

    /* Configure supervisor trap vector and scratch
    (supervisor stack pointer) */
    asm volatile("la    %0, trap_handler \n"
                 "csrw  stvec, %0        \n"
                 : "=&r"(temp));

    /* Enable waking from WFI on supervisor software interrupts (IPIs) and bus error interrupts
    But disable interrupts globally so that they *do not* trap to the trap handler */
    /* TODO: create and use proper macros from interrupts.h */
    asm volatile("csrci sstatus, 0x2\n"
                 "csrw  sie, %0\n"
                 :
                 : "r"((1 << SUPERVISOR_SOFTWARE_INTERRUPT) | (1 << BUS_ERROR_INTERRUPT)));

    const uint32_t hart_id = get_hart_id();

    /* Launch Dispatcher and Workers */
    if (hart_id == DISPATCHER_BASE_HART_ID)
    {
        /* Initialize UART logging params */
        Log_Init();

        Dispatcher_Launch(hart_id);
    }
    else if (hart_id == SPW_BASE_HART_ID)
    {
        /* Spin wait till dispatcher initialization is complete */
        local_spinwait_wait(&Launch_Wait, 1, 0);
        SPW_Launch(hart_id);
    }
    else if ((hart_id >= SQW_HP_BASE_HART_ID) && (hart_id < SQW_HP_MAX_HART_ID) &&
             !EVEN_HART(hart_id))
    {
        /* Spin wait till dispatcher initialization is complete */
        local_spinwait_wait(&Launch_Wait, 1, 0);
        SQW_HP_Launch((hart_id - SQW_HP_BASE_HART_ID) / HARTS_PER_MINION);
    }
    else if ((hart_id >= SQW_BASE_HART_ID) && (hart_id < SQW_MAX_HART_ID) && EVEN_HART(hart_id))
    {
        /* Spin wait till dispatcher initialization is complete */
        local_spinwait_wait(&Launch_Wait, 1, 0);
        SQW_Launch((hart_id - SQW_BASE_HART_ID) / HARTS_PER_MINION);
    }
    else if ((hart_id >= KW_BASE_HART_ID) && (hart_id < KW_MAX_HART_ID) && EVEN_HART(hart_id))
    {
        /* Spin wait till dispatcher initialization is complete */
        local_spinwait_wait(&Launch_Wait, 1, 0);
        KW_Launch((hart_id - KW_BASE_HART_ID) / HARTS_PER_MINION);
    }
    else if ((hart_id >= DMAW_BASE_HART_ID) && (hart_id < DMAW_MAX_HART_ID) && EVEN_HART(hart_id))
    {
        /* Spin wait till dispatcher initialization is complete */
        local_spinwait_wait(&Launch_Wait, 1, 0);
        DMAW_Launch(hart_id);
    }
    else
    {
        while (1)
        {
            asm volatile("wfi");
        }
    }

    /* Warning: Should never reach this point */
    while (1)
    {
        asm volatile("wfi");
    }
}
