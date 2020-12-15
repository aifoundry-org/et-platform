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
*       This file consists the implementation for interrupt 
*       registration and management services
*
*   FUNCTIONS
*
*       main2
*
***********************************************************************/
#include "drivers/interrupts.h"
#include "io.h"
#include "etsoc_hal/inc/hal_device.h"
#include "etsoc_hal/inc/pu_plic.h"

/*! \var void (*Interrupt_Vector_Table[])(void)
    \brief Global Vector Table
    \warning Not thread safe!
*/
void (*Interrupt_Vector_Table[PU_PLIC_INTR_CNT])(void) = { NULL };

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
*       Interrupt_Init
*  
*   DESCRIPTION
*
*       Initialize interrupt resources
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
void Interrupt_Init(void)
{
    /*Set thresholds to not mask any interrupts*/
    iowrite32(R_PU_PLIC_BASEADDR + PU_PLIC_THRESHOLD_T11_ADDRESS, 0);
}

/************************************************************************
*
*   FUNCTION
*
*       Interrupt_Enable
*  
*   DESCRIPTION
*
*       Enable Interrupt resource
*
*   INPUTS
*
*       interrupt_t     interrupt vector
*       uint32_t        priority
*       void (*isr)     function pointer to interrupt handler
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Interrupt_Enable(interrupt_t interrupt, uint32_t priority, 
    void (*isr)(void))
{
    Interrupt_Vector_Table[interrupt] = isr;
    
    plic_enable_interrupt(
        (volatile uint32_t *const)
        (R_PU_PLIC_BASEADDR + PU_PLIC_PRIORITY_0_ADDRESS), 
        (volatile uint32_t *const)
        (R_PU_PLIC_BASEADDR + PU_PLIC_ENABLE_T11_R0_ADDRESS), 
        (uint32_t)interrupt, priority);
}

/************************************************************************
*
*   FUNCTION
*
*       Interrupt_Disable
*  
*   DESCRIPTION
*
*       Disable Interrupt resource
*
*   INPUTS
*
*       interrupt_t     interrupt vector
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Interrupt_Disable(interrupt_t interrupt)
{
    // TODO FIXME target enumeration is a mystery, t0 is wrong
    plic_disable_interrupt(
        (volatile uint32_t *const)
        (R_PU_PLIC_BASEADDR + PU_PLIC_PRIORITY_0_ADDRESS),
        (volatile uint32_t *const)
        (R_PU_PLIC_BASEADDR + PU_PLIC_ENABLE_T11_R0_ADDRESS),
        (uint32_t)interrupt);
    
    Interrupt_Vector_Table[interrupt] = NULL;
}