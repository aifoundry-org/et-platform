#include "interrupt.h"
#include "hal_device.h"
#include "pu_plic.h"

#include <stddef.h>

#define PU_PLIC ((volatile Pu_plic_t* const)R_PU_PLIC_BASEADDR)
#define PRIORITY_MASK 0x7U

static void plicEnableInterrupt(
    volatile uint32_t* const basePriorityReg,
    volatile uint32_t* const baseEnableReg,
    uint32_t intID,
    uint32_t priority);

static void plicDisableInterrupt(
    volatile uint32_t* const basePriorityReg,
    volatile uint32_t* const baseEnableReg,
    uint32_t intID);

void (*vectorTable[PU_PLIC_INTR_CNT])(void) = {NULL};

void* pullVectorTable = vectorTable;

void INT_init(void)
{
    //Set thresholds to not mask any interrupts
    PU_PLIC->threshold_t11.R = 0;
}

void INT_enableInterrupt(interrupt_t interrupt, uint32_t priority, void (*isr)(void))
{
    vectorTable[interrupt] = isr;
    plicEnableInterrupt(&PU_PLIC->priority_0.R, &PU_PLIC->enable_t11_r0.R, (uint32_t)interrupt, priority);
}

void INT_disableInterrupt(interrupt_t interrupt)
{
    // TODO FIXME target enumeration is a mystery, t0 is wrong
    plicDisableInterrupt(&PU_PLIC->priority_0.R, &PU_PLIC->enable_t11_r0.R, (uint32_t)interrupt);
    vectorTable[interrupt] = NULL;
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
