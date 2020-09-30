/*-------------------------------------------------------------------------
* Copyright (C) 2019, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#include "interrupt.h"
#include "io.h"

#include "FreeRTOS.h"
#include "etsoc_hal/inc/pu_plic.h"
#include "etsoc_hal/inc/spio_plic.h"
#include "etsoc_hal/inc/hal_device.h"
#include "task.h"

#include <stdio.h>
#include <inttypes.h>

//TODO FIXME generate these. Copied by hand by wearle 5/3/2019.
#define SPIO_PLIC_INTR_SRC_CNT                156
#define PU_PLIC_INTR_SRC_CNT                   42

#define SPIO_PLIC R_SP_PLIC_BASEADDR
#define PU_PLIC   R_PU_PLIC_BASEADDR

#define PRIORITY_MASK 0x7U

void (*vectorTable[SPIO_PLIC_INTR_SRC_CNT])(void);
void* pullVectorTable = vectorTable;

static void (*pu_plicVectorTable[PU_PLIC_INTR_SRC_CNT])(void);
static uint16_t pu_plicEnabledIntCnt;

static void plicEnableInterrupt(
    volatile uint32_t* const basePriorityReg,
    volatile uint32_t* const baseEnableReg,
    uint32_t intID,
    uint32_t priority);

static void plicDisableInterrupt(
    volatile uint32_t* const basePriorityReg,
    volatile uint32_t* const baseEnableReg,
    uint32_t intID);

static void pu_plicISR(void);

void INT_init(void)
{
    taskENTER_CRITICAL();
    pu_plicEnabledIntCnt = 0;

    //Set thresholds to not mask any interrupts

    //SPIO PLIC target 0 is the SP HART0 machine-mode external interrupt
    iowrite32(SPIO_PLIC + SPIO_PLIC_THRESHOLD_T0_ADDRESS, 0);

    //PU PLIC target 0 is the SPIO PLIC SPIO_PLIC_PU_PLIC0_INTR input
    iowrite32(PU_PLIC + PU_PLIC_THRESHOLD_T0_ADDRESS, 0);

    taskEXIT_CRITICAL();
}

void INT_enableInterrupt(interrupt_t interrupt, uint32_t priority, void (*isr)(void))
{
    taskENTER_CRITICAL();

    if (interrupt < PU_PLIC_NO_INTERRUPT_INTR)
    {
        //Interrupt comes from SPIO_PLIC. FreeRTOS's freertos_risc_v_trap_handler will call ISR by indexing into vectorTable.
        vectorTable[interrupt] = isr;

        plicEnableInterrupt(
            (volatile uint32_t* const)(SPIO_PLIC + SPIO_PLIC_PRIORITY_0_ADDRESS),
            (volatile uint32_t* const)(SPIO_PLIC + SPIO_PLIC_ENABLE_T0_R0_ADDRESS),
            (uint32_t)interrupt, priority);
    }
    else
    {
        //Interrupt from PU_PLIC and is forwarded to SP through SPIO_PLIC
        uint32_t pu_plicIntID = (uint32_t)interrupt - PU_PLIC_NO_INTERRUPT_INTR;

        pu_plicVectorTable[pu_plicIntID] = isr;
        plicEnableInterrupt(
            (volatile uint32_t* const)(PU_PLIC + PU_PLIC_PRIORITY_0_ADDRESS),
            (volatile uint32_t* const)(PU_PLIC + PU_PLIC_ENABLE_T0_R0_ADDRESS),
            pu_plicIntID, priority);

        vectorTable[SPIO_PLIC_PU_PLIC0_INTR] = &pu_plicISR;
        plicEnableInterrupt(
            (volatile uint32_t* const)(SPIO_PLIC + SPIO_PLIC_PRIORITY_0_ADDRESS),
            (volatile uint32_t* const)(SPIO_PLIC + SPIO_PLIC_ENABLE_T0_R0_ADDRESS),
            SPIO_PLIC_PU_PLIC0_INTR, 7); //TODO: appropriate priority?

        ++pu_plicEnabledIntCnt;
    }

    taskEXIT_CRITICAL();
}

void INT_disableInterrupt(interrupt_t interrupt)
{
    taskENTER_CRITICAL();

    if (interrupt < PU_PLIC_NO_INTERRUPT_INTR)
    {
        //Interrupt comes from SPIO_PLIC
        plicDisableInterrupt(
            (volatile uint32_t* const)(SPIO_PLIC + SPIO_PLIC_PRIORITY_0_ADDRESS),
            (volatile uint32_t* const)(SPIO_PLIC + SPIO_PLIC_ENABLE_T0_R0_ADDRESS),
            (uint32_t)interrupt);
        vectorTable[interrupt] = NULL;
    }
    else
    {
        //Interrupt comes from PU_PLIC
        uint32_t pu_plicIntID = (uint32_t)interrupt - PU_PLIC_NO_INTERRUPT_INTR;

        plicDisableInterrupt(
            (volatile uint32_t* const)(PU_PLIC + PU_PLIC_PRIORITY_0_ADDRESS),
            (volatile uint32_t* const)(PU_PLIC + PU_PLIC_ENABLE_T0_R0_ADDRESS),
            (uint32_t)pu_plicIntID);
        pu_plicVectorTable[pu_plicIntID] = NULL;

        --pu_plicEnabledIntCnt;

        if (pu_plicEnabledIntCnt == 0)
        {
            plicDisableInterrupt(
                (volatile uint32_t* const)(SPIO_PLIC + SPIO_PLIC_PRIORITY_0_ADDRESS),
                (volatile uint32_t* const)(SPIO_PLIC + SPIO_PLIC_ENABLE_T0_R0_ADDRESS),
                SPIO_PLIC_PU_PLIC0_INTR);
            vectorTable[SPIO_PLIC_PU_PLIC0_INTR] = NULL;
        }
    }

    taskEXIT_CRITICAL();
}

static void plicEnableInterrupt(
    volatile uint32_t* const basePriorityReg,
    volatile uint32_t* const baseEnableReg,
    uint32_t intID,
    uint32_t priority)
{
    volatile uint32_t* const priorityReg = basePriorityReg + intID;
    volatile uint32_t* const enableReg = baseEnableReg + (intID / 32);
    const uint32_t enableMask = 1U << (intID % 32);

    *priorityReg &= ~PRIORITY_MASK;
    *priorityReg |= priority & PRIORITY_MASK;
    *enableReg |= enableMask;
}

static void plicDisableInterrupt(
    volatile uint32_t* const basePriorityReg,
    volatile uint32_t* const baseEnableReg,
    uint32_t intID)
{
    volatile uint32_t* const priorityReg = basePriorityReg + intID;
    volatile uint32_t* const enableReg = baseEnableReg + (intID / 32);
    const uint32_t enableMask = 1U << (intID % 32);

    *enableReg &= ~enableMask; // disable first
    *priorityReg = 0;
}

static void pu_plicISR(void)
{
    //Assumption: taskENTER_CRITICAL is met. This is called from machine interrupt context (highest HART priority), and the RTOS is single-threaded
    //Assumption: FreeRTOS's freertos_risc_v_trap_handler reads SP_PLIC's maxID reg before vectoring here, claiming the SP_PLIC int

    //The SP PLIC got an IRQ from the PU PLIC. Claim it.
    uint32_t pu_plicIntID = ioread32(PU_PLIC + PU_PLIC_MAXID_T0_ADDRESS);

    //An ID of 0 means the PLIC signaled with no interrupts pending
    if (pu_plicIntID == 0) return;

    //Dispatch it
    (*pu_plicVectorTable[pu_plicIntID])();

    //Tell the PU_PLIC the interrupt is complete and it can signal again
    //Important: it needs to match the int ID read above
    iowrite32(PU_PLIC + PU_PLIC_MAXID_T0_ADDRESS, pu_plicIntID);

    //Assumption: FreeRTOS's freertos_risc_v_trap_handler will write the SP_PLIC's maxID reg after this returns,
}
