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
#include "workers/cqw.h"
#include "services/lock.h"
#include "hart.h"
#include "atomic.h"

/*! \var lock_t System_Level_Sync_lock
    \brief Global system levele sync spin lock
    \warning Not thread safe!
*/
lock_t Gbl_System_level_Sync_Lock;

void main2(void);

void main2(void)
{
    uint64_t temp;

    /* Configure supervisor trap vector and scratch 
    (supervisor stack pointer) */
    asm volatile("la    %0, trap_handler \n"
                 "csrw  stvec, %0        \n"
                 : "=&r"(temp));

    const uint32_t hart_id = get_hart_id();

    /* Launch Dispatcher and Workers */
    if (hart_id == DISPATCHER_BASE_HART_ID)
    {
        Dispatcher_Launch(hart_id);
    } 
    else if ((hart_id >= SQW_BASE_HART_ID) && 
            (hart_id < SQW_MAX_HART_ID))
    {
        SQW_Launch(hart_id - SQW_BASE_HART_ID);
    } 
    else if ((hart_id >= KW_BASE_HART_ID) && 
            (hart_id < KW_MAX_HART_ID))
    {
        KW_Launch(hart_id - KW_BASE_HART_ID);
    } 
    else if ((hart_id >= DMAW_BASE_HART_ID) && 
            (hart_id < DMAW_MAX_HART_ID))
    {
        DMAW_Launch(hart_id);
    }
    else if ((hart_id >= CQW_BASE_HART_ID) && 
            (hart_id < CQW_MAX_HART_ID))
    {
        CQW_Launch(hart_id);
    }
    else
    {
        while (1) {
            asm volatile("wfi");
        }
    }
}