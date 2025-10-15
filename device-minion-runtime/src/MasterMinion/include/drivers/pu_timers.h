/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file pu_timers.h
    \brief A C header that defines the PU Timers Driver's public
    interfaces
*/
/***********************************************************************/
#ifndef PU_TIMERS_H
#define PU_TIMERS_H

#include <stdint.h>

/*! \fn void PU_Timer_Init(void *timeout_callback_fn, uint32_t timeout)
    \brief Initialize Driver, and starts the PU Timer channel 0 from counting down
    \param timeout_callback_fn Timout callback function.
    \param timeout Timeout.
    \returns SUCCESS or ERROR
*/
void PU_Timer_Init(void (*timeout_callback_fn)(void), uint32_t timeout);

/*! \fn void PU_Timer_Get_Current_Value(void)
    \brief Gets the remaining value in the count down timer
    \returns Current remaining value
*/
uint32_t PU_Timer_Get_Current_Value(void);

/*! \fn void PU_Timer_Stop(void)
    \brief Stops a Timer channel from counting down
    \returns none
*/
void PU_Timer_Stop(void);

/*! \fn void PU_Timer_Interrupt_Clear(void)
    \brief Clear Timer channel interrupt
    \returns none
*/
void PU_Timer_Interrupt_Clear(void);

#endif /* TINMERS_H */


