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
/*! \file pu_timers.c
    \brief A C module that implements the PU Timers Driver

    Public interfaces:
        PU_Timer_Init
        PU_Timer_Stop
        PU_Timer_Get_Current_Value
        PU_Timer_Interrupt_Clear
*/
/***********************************************************************/
#include <stddef.h>

/* mm_rt_svcs */
#include <etsoc/isa/io.h>
#include <etsoc/isa/atomic.h>

/* etsoc_hal */
#include <hwinc/pu_timer.h>
#include <hwinc/hal_device.h>
#include <hwinc/pu_plic.h>

/* mm specific headers */
#include "services/log.h"
#include "drivers/pu_timers.h"
#include "drivers/plic.h"

/*! \def TIMERS_INT_PRIORITY
    \brief Macro that provides the TIMER interrupt priority.
*/
#define TIMERS_INT_PRIORITY 1

/*! \fn pu_timer_enable
    \brief Helper function for enabling a specific TIMER channel to start counting down
*/
static inline void pu_timer_enable(uint32_t timeout_value)
{
    uint32_t control_reg = 0;

    /* From PRM: Before writing to a TimerNLoadCount register, you must disable the timer by writing a “0” to
    the timer enable bit of TimerNControlReg in order to avoid potential synchronization problems */
    iowrite32(R_PU_TIMER_BASEADDR + TIMERS_TIMER1CONTROLREG_OFFSET,
        TIMERS_TIMER1CONTROLREG_TIMER_ENABLE_SET(
            TIMERS_TIMER1CONTROLREG_TIMER_ENABLE_TIMER_ENABLE_DISABLE));
    /* Program the timer mode and set it to unmasked interrupts */
    control_reg |= TIMERS_TIMER1CONTROLREG_TIMER_MODE_SET(
                       TIMERS_TIMER1CONTROLREG_TIMER_MODE_TIMER_MODE_USER_DEFINED) |
                   TIMERS_TIMER1CONTROLREG_TIMER_INTERRUPT_MASK_SET(
                       TIMERS_TIMER1CONTROLREG_TIMER_INTERRUPT_MASK_TIMER_INTERRUPT_MASK_UNMASKED);
    iowrite32(R_PU_TIMER_BASEADDR + TIMERS_TIMER1CONTROLREG_OFFSET, control_reg);
    /* Program the timeout value */
    iowrite32(R_PU_TIMER_BASEADDR + TIMERS_TIMER1LOADCOUNT_OFFSET, timeout_value);
    /* Enable timer */
    control_reg |= TIMERS_TIMER1CONTROLREG_TIMER_ENABLE_SET(
        TIMERS_TIMER1CONTROLREG_TIMER_ENABLE_TIMER_ENABLE_ENABLED);
    iowrite32(R_PU_TIMER_BASEADDR + TIMERS_TIMER1CONTROLREG_OFFSET, control_reg);
}

/*! \fn pu_timer_disable
    \brief Helper function for disabling a specific TIMER channel
*/
static inline void pu_timer_disable(void)
{
    iowrite32(R_PU_TIMER_BASEADDR + TIMERS_TIMER1CONTROLREG_OFFSET,
        TIMERS_TIMER1CONTROLREG_TIMER_ENABLE_SET(
            TIMERS_TIMER1CONTROLREG_TIMER_ENABLE_TIMER_ENABLE_DISABLE));
}

/************************************************************************
*
*   FUNCTION
*
*       PU_Timer_Init
*
*   DESCRIPTION
*
*       Initial the PU Timer Driver and kick start the channel 0 PU TIMER
*
*   INPUTS
*
*       void (*timeout_callback_fn)(void) Callback function when Timer expires
*       uint32_t   timeout                Timeout value to count down to
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void PU_Timer_Init(void (*timeout_callback_fn)(void), uint32_t timeout)
{
    /* Load Timer Channel with Timeout value and start counting down */
    pu_timer_enable(timeout);

    /* Register Callback to PLIC Interrupt when Timer fires */
    PLIC_RegisterHandler(
        PU_PLIC_TIMER0_INTR_ID, TIMERS_INT_PRIORITY, (void (*)(uint32_t))timeout_callback_fn);
}

/************************************************************************
*
*   FUNCTION
*
*       PU_Timer_Stop
*
*   DESCRIPTION
*
*       Disable TIMER channel
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
void PU_Timer_Stop(void)
{
    /* Disable Timer on Channel */
    pu_timer_disable();

    /* Unregister PLIC callback */
    PLIC_UnregisterHandler(PU_PLIC_TIMER0_INTR_ID);
}

/************************************************************************
*
*   FUNCTION
*
*       PU_Timer_Get_Current_Value
*
*   DESCRIPTION
*
*       Gets the remaining value in the count down timer
*
*   INPUTS
*
*       void
*
*   OUTPUTS
*
*       uint32_t Remaining value
*
***********************************************************************/
uint32_t PU_Timer_Get_Current_Value(void)
{
    return ioread32(R_PU_TIMER_BASEADDR + TIMERS_TIMER1CURRENTVAL_ADDRESS);
}

/************************************************************************
*
*   FUNCTION
*
*       PU_Timer_Interrupt_Clear
*
*   DESCRIPTION
*
*       Helper function for clearing TIMER channel interrupt
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
void PU_Timer_Interrupt_Clear(void)
{
    ioread32(R_PU_TIMER_BASEADDR + TIMERS_TIMER1EOI_OFFSET);
}
