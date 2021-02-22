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
*       main2
*
***********************************************************************/
#include "common_defs.h"
#include "dispatcher/dispatcher.h"
#include "workers/sqw.h"
#include "workers/kw.h"
#include "workers/dmaw.h"
#include "services/lock.h"
#include "services/log1.h"
#include "sync.h"
#include "hart.h"
#include "atomic.h"
#include "riscv_encoding.h"

/*! \var spinlock_t Launch_Wait
    \brief Spinlock used to let the FW workers continue.
    Released by the Dispatcher.
    \warning Not thread safe!
*/
spinlock_t Launch_Wait = {0};

/*! \fn static inline void main2_early_init(void)
    \brief Function to do early initialization
    (once) of components before any Minion is
    launched.
*/

void main2(void);

void main2(void)
{
    uint64_t temp;

    /* Configure supervisor trap vector and scratch
    (supervisor stack pointer) */
    asm volatile("la    %0, trap_handler \n"
                 "csrw  stvec, %0        \n"
                 : "=&r"(temp));

    /* Enable waking from WFI on supervisor software interrupts (IPIs).
    But disable interrupts globally so that they *do not* trap to the trap handler */
    /* TODO: create and use proper macros from interrupts.h */
    asm volatile("csrci sstatus, 0x2\n"
                 "csrw  sie, %0\n"
                 : : "I" (1 << SUPERVISOR_SOFTWARE_INTERRUPT));

    const uint32_t hart_id = get_hart_id();

    /* Launch Dispatcher and Workers */
    if (hart_id == DISPATCHER_BASE_HART_ID)
    {
        /* Initialize UART logging params */
        Log_Init(LOG_LEVEL_CRITICAL);

        Dispatcher_Launch(hart_id);
    }
    else if ((hart_id >= SQW_BASE_HART_ID) &&
            (hart_id < SQW_MAX_HART_ID))
    {
        /* Spin wait till dispatcher initialization is complete */
        local_spinwait_wait(&Launch_Wait, 1);
        SQW_Launch(hart_id, (hart_id - SQW_BASE_HART_ID));
    }
    else if ((hart_id >= KW_BASE_HART_ID) &&
            (hart_id < KW_MAX_HART_ID))
    {
        /* Spin wait till dispatcher initialization is complete */
        local_spinwait_wait(&Launch_Wait, 1);
        KW_Launch(hart_id, hart_id - KW_BASE_HART_ID);
    }
    else if ((hart_id >= DMAW_BASE_HART_ID) &&
            (hart_id < DMAW_MAX_HART_ID))
    {
        /* Spin wait till dispatcher initialization is complete */
        local_spinwait_wait(&Launch_Wait, 1);
        DMAW_Launch(hart_id);
    }
    else
    {
        while (1) {
            asm volatile("wfi");
        }
    }
}
