/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/***********************************************************************/
/*! \file spw.c
    \brief A C module that implements the Service Processor Queue Worker's
    public and private interfaces.
    Public interfaces:
        SPW_Launch

*/
/***********************************************************************/
/* mm_rt_svcs */
#include <etsoc/drivers/pmu/pmu.h>
#include <etsoc/isa/riscv_encoding.h>

/* mm specific headers */
#include "workers/spw.h"
#include "services/log.h"
#include "services/sp_iface.h"

/************************************************************************
*
*   FUNCTION
*
*       SPW_Launch
*
*   DESCRIPTION
*
*       Launch a Service Processor Worker on HART ID requested
*
*   INPUTS
*
*       uint32_t   HART ID to launch the Service Processor worker
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void SPW_Launch(uint32_t hart_id)
{
    uint64_t sip;

    Log_Write(LOG_LEVEL_INFO, "SPW:launched on H%d\r\n", hart_id);

    /* Reset PMC cycles counter for all Harts
    (can be even or odd depending upong hart ID) in the Neighbourhood */
    PMC_RESET_CYCLES_COUNTER;

    while(1)
    {
        /* Wait for an interrupt */
        asm volatile("wfi");

        /* Read pending interrupts */
        SUPERVISOR_PENDING_INTERRUPTS(sip);

        /* We are only interested in IPIs */
        if(!(sip & (1 << SUPERVISOR_SOFTWARE_INTERRUPT)))
        {
            continue;
        }

        /* Clear IPI pending interrupt */
        asm volatile("csrci sip, %0" : : "I"(1 << SUPERVISOR_SOFTWARE_INTERRUPT));

        Log_Write(LOG_LEVEL_DEBUG, "SPW:IPI received!\r\n");

        /* Process any pending SP commands */
        SP_Iface_Processing();

    } /* loop forever */

    /* will not return */
}
