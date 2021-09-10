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
        PU_Timer_Start
        PU_Timer_Stop
        PU_Timer_Interrupt_Clear
*/
/***********************************************************************/
#include "drivers/pu_timers.h"
#include "drivers/plic.h"
#include "io.h"
#include "device-common/atomic.h"
#include "hwinc/pu_timer.h"
#include "hwinc/hal_device.h"
#include "hwinc/pu_plic.h"
#include "services/log.h"

#include <stddef.h>

/*! \def TIMERS_INT_PRIORITY
    \brief Macro that provides the TIMER interrupt priority.
*/
#define TIMERS_INT_PRIORITY   1

/*! \fn PU_Timer_Enable
    \brief Helper function for enabling a specific TIMER channel to start counting down
*/
static inline void PU_Timer_Enable(uint32_t timeout_value)
{
    iowrite32(R_PU_TIMER_BASEADDR + TIMERS_TIMER1LOADCOUNT_OFFSET, timeout_value);
    iowrite32(R_PU_TIMER_BASEADDR + TIMERS_TIMER1CONTROLREG_OFFSET,
                    TIMERS_TIMER1CONTROLREG_TIMER_ENABLE_SET
                    (TIMERS_TIMER1CONTROLREG_TIMER_ENABLE_TIMER_ENABLE_ENABLED) |
                    TIMERS_TIMER1CONTROLREG_TIMER_MODE_SET
                    (TIMERS_TIMER1CONTROLREG_TIMER_MODE_TIMER_MODE_USER_DEFINED));
}

/*! \fn PU_Timer_Disable
    \brief Helper function for disabling a specific TIMER channel
*/
static inline void PU_Timer_Disable(void)
{
    iowrite32(R_PU_TIMER_BASEADDR + TIMERS_TIMER1CONTROLREG_OFFSET,
                    TIMERS_TIMER1CONTROLREG_TIMER_ENABLE_SET
                    (TIMERS_TIMER1CONTROLREG_TIMER_ENABLE_TIMER_ENABLE_DISABLE));
}

/*! \fn PU_Timer_Interrupt_Clear
    \brief Helper function for clearing TIMER channel interrupt
*/
void PU_Timer_Interrupt_Clear(void)
{
    ioread32(R_PU_TIMER_BASEADDR + TIMERS_TIMER1EOI_OFFSET);
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
*
***********************************************************************/
void PU_Timer_Init(void (*timeout_callback_fn)(void), uint32_t timeout)
{
    /* Register Callback to PLIC Interrupt when Timer fires */
    PLIC_RegisterHandler(PU_PLIC_TIMER0_INTR_ID, TIMERS_INT_PRIORITY,
                         (void (*)(uint32_t))timeout_callback_fn);

    /* Load Timer Channel with Timeout value and start counting down */
    PU_Timer_Enable(timeout);
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
    PU_Timer_Disable();

    /* Unregister PLIC callback */
    PLIC_UnregisterHandler(PU_PLIC_TIMER0_INTR_ID);

}
