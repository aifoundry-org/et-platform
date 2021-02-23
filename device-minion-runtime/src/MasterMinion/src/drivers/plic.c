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
************************************************************************/
/*! \file interrupts.c
    \brief A C module that implements the PLIC Driver

    Public interfaces:
        PLIC_Init
        PLIC_RegisterHandler
        PLIC_UnregisterHandler
        PLIC_Dispatch
*/
/***********************************************************************/
#include "drivers/plic.h"
#include "io.h"
#include "etsoc_hal/inc/hal_device.h"
#include "etsoc_hal/inc/pu_plic.h"

#include <stddef.h>

/*! \var void (*handlerTable[PU_PLIC_INTR_SRC_CNT])(uint32_t intID)
    \brief Global PLIC Handler Table
    \warning Not thread safe!
*/
static void (*handlerTable[PU_PLIC_INTR_SRC_CNT])(uint32_t intID) = { NULL };

static void plic_enable_interrupt(volatile uint32_t *const basePriorityReg,
    volatile uint32_t *const baseEnableReg, uint32_t intID,
    uint32_t priority)
{
    volatile uint32_t *const priorityReg = basePriorityReg + intID;
    volatile uint32_t *const enableReg = baseEnableReg + (intID / 32);
    const uint32_t enableMask = 1U << (intID % 32);

    *priorityReg &= ~0x7U;
    *priorityReg |= priority & 0x7U;
    *enableReg |= enableMask;
}

static void plic_disable_interrupt(volatile uint32_t *const basePriorityReg,
    volatile uint32_t *const baseEnableReg, uint32_t intID)
{
    volatile uint32_t *const priorityReg = basePriorityReg + intID;
    volatile uint32_t *const enableReg = baseEnableReg + (intID / 32);
    const uint32_t enableMask = 1U << (intID % 32);

    *enableReg &= ~enableMask; // disable first
    *priorityReg = 0;
}

/************************************************************************
*
*   FUNCTION
*
*       PLIC_Init
*
*   DESCRIPTION
*
*       Initialize PLIC driver
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void PLIC_Init(void)
{
    /*Set thresholds to not mask any interrupts*/
    iowrite32(R_PU_PLIC_BASEADDR + PU_PLIC_THRESHOLD_T11_ADDRESS, 0);
}

/************************************************************************
*
*   FUNCTION
*
*       PLIC_RegisterHandler
*
*   DESCRIPTION
*
*       Registers a handler for a given interrupt source
*
*   INPUTS
*
*       uint32_t                        interrupt ID
*       uint32_t                        priority
*       void (*handler)(uint32_t intID) function pointer to handler routine
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void PLIC_RegisterHandler(uint32_t intID, uint32_t priority,
    void (*handler)(uint32_t intID))
{
    handlerTable[intID] = handler;

    plic_enable_interrupt(
        (volatile uint32_t *const)
        (R_PU_PLIC_BASEADDR + PU_PLIC_PRIORITY_0_ADDRESS),
        (volatile uint32_t *const)
        (R_PU_PLIC_BASEADDR + PU_PLIC_ENABLE_T11_R0_ADDRESS),
        intID, priority);
}

/************************************************************************
*
*   FUNCTION
*
*       PLIC_UnregisterHandler
*
*   DESCRIPTION
*
*       Unregisters a handler for a given interrupt source
*
*   INPUTS
*
*       uint32_t        interrupt ID
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void PLIC_UnregisterHandler(uint32_t intID)
{
    // TODO FIXME target enumeration is a mystery, t0 is wrong
    plic_disable_interrupt(
        (volatile uint32_t *const)
        (R_PU_PLIC_BASEADDR + PU_PLIC_PRIORITY_0_ADDRESS),
        (volatile uint32_t *const)
        (R_PU_PLIC_BASEADDR + PU_PLIC_ENABLE_T11_R0_ADDRESS),
        intID);

    handlerTable[intID] = NULL;
}

/************************************************************************
*
*   FUNCTION
*
*       PLIC_Dispatch
*
*   DESCRIPTION
*
*       Dispatches all pending interrupts
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void PLIC_Dispatch(void)
{
    uint32_t maxID;

    while (1) {
        /* Load MaxID of Target 11 (Minion supervisor level interrupt) to claim the interrupt */
        maxID = ioread32(R_PU_PLIC_BASEADDR + PU_PLIC_MAXID_T11_ADDRESS);

        /* If MaxID is zero, there are no further interrupts */
        if(maxID == 0)
        {
            break;
        }

        /* If we have a registered handler, call it */
        if (handlerTable[maxID])
        {
            handlerTable[maxID](maxID);
        }

        /* Write MaxID to indicate the interrupt has been serviced */
        iowrite32(R_PU_PLIC_BASEADDR + PU_PLIC_MAXID_T11_ADDRESS, maxID);
    }
}
