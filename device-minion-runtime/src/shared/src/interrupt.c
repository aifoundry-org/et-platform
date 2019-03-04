#include "interrupt.h"

#include "FreeRTOS.h"
#include "task.h"

// Table of function pointers that take and return void
void (*vectorTable[NUM_INTERRUPTS])(void);

void INT_enableInterrupt(interrupt_t interrupt, uint32_t priority, void (*function)(void))
{
    // TODO FIXME clean this up once we have PLIC.h
    volatile uint32_t* const pPriority = (volatile uint32_t* const)(PLIC_BASE_ADDRESS + 0x0U    + (interrupt * 4U));
    volatile uint32_t* const pEnable   = (volatile uint32_t* const)(PLIC_BASE_ADDRESS + 0x2000U + ((interrupt / 32) * 4U));
    const uint32_t enableBit = 1U << (interrupt % 32);

    taskENTER_CRITICAL();
    *pPriority = priority & 0x7U; // priority is bits [2:0]
    vectorTable[interrupt] = function;
    *pEnable |= enableBit; // enable last
    taskEXIT_CRITICAL();
}

void INT_disableInterrupt(interrupt_t interrupt)
{
    // TODO FIXME clean this up once we have PLIC.h
    volatile uint32_t* const pPriority = (volatile uint32_t* const)(PLIC_BASE_ADDRESS + 0x0U    + (interrupt * 4U));
    volatile uint32_t* const pEnable   = (volatile uint32_t* const)(PLIC_BASE_ADDRESS + 0x2000U + ((interrupt / 32) * 4U));
    const uint32_t enableBit = 1U << (interrupt % 32);

    taskENTER_CRITICAL();
    *pEnable &= ~enableBit; // disable first
    *pPriority = 0;
    vectorTable[interrupt] = 0;
    taskEXIT_CRITICAL();
}
