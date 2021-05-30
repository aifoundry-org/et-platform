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
/*! \file pu_timers.h
    \brief A C header that defines the PU Timers Driver's public
    interfaces
*/
/***********************************************************************/
#ifndef PU_TIMERS_H
#define PU_TIMERS_H

#include <stdint.h>

/*! \fn void PU_Timers_Init(void (*timeout_callback_fn)(void), uint32_t timeout)
    \brief Initialize Driver, and starts the PU Timer channel 0 from counting down
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


