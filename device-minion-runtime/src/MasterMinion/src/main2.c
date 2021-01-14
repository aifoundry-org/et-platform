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

/*! \var Launch_Lock
    \brief Global Host to MM submission
    queues interface. Locks initialized
    to acquired state.
    \warning Not thread safe!
*/
spinlock_t Launch_Lock = {0};

/*! \var Early_Init_Done
    \brief Global variable to do early initialization.
*/
static uint32_t Early_Init_Done __attribute__((aligned(64))) = 0;

/*! \fn static inline void main2_early_init(void)
    \brief Function to do early initialization 
    (once) of components before any Minion is 
    launched.
*/
static inline void main2_early_init(void)
{
    if (atomic_or_global_32(&Early_Init_Done, 1U) == 0U)
    {
        asm volatile("fence\n" ::: "memory");

        /* Init launch lock to acquired state */
        init_local_spinlock(&Launch_Lock, 1);

        /* Initialize UART logging params */
        Log_Init(LOG_LEVEL_DEBUG);
    }
    asm volatile("fence\n" ::: "memory");
}

void main2(void);

void main2(void)
{
    uint64_t temp;

    /* Configure supervisor trap vector and scratch 
    (supervisor stack pointer) */
    asm volatile("la    %0, trap_handler \n"
                 "csrw  stvec, %0        \n"
                 : "=&r"(temp));

    /* Call the early initialization function. */
    main2_early_init();

    const uint32_t hart_id = get_hart_id();

    /* Launch Dispatcher and Workers */
    if (hart_id == DISPATCHER_BASE_HART_ID)
    {
        Dispatcher_Launch(hart_id);
    } 
    else if ((hart_id >= SQW_BASE_HART_ID) && 
            (hart_id < SQW_MAX_HART_ID))
    {
        /* Spin wait till dispatcher initialization is complete */
        acquire_local_spinlock(&Launch_Lock);
        SQW_Launch(hart_id, (hart_id - SQW_BASE_HART_ID));
    }
    #if 0
    else if ((hart_id >= KW_BASE_HART_ID) && 
            (hart_id < KW_MAX_HART_ID))
    {
        /* Spin wait till dispatcher initialization is complete */
        acquire_local_spinlock(&Launch_Lock);
        KW_Launch(hart_id, hart_id - KW_BASE_HART_ID);
    }
    #endif
    else if ((hart_id >= DMAW_BASE_HART_ID) && 
            (hart_id < DMAW_MAX_HART_ID))
    {
        /* Spin wait till dispatcher initialization is complete */
        acquire_local_spinlock(&Launch_Lock);
        DMAW_Launch(hart_id);
    }
    else
    {
        while (1) {
            asm volatile("wfi");
        }
    }
}
