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
/***********************************************************************/
/*! \file sw_timer.h
    \brief A C header that defines the SW Timer public interfaces.
*/
/***********************************************************************/
#ifndef SW_TIMER_H
#define SW_TIMER_H

#include <stdint.h>

/*! \def SW_TIMER_MAX_SLOTS
    \brief Maximum SW Timer slots allowed to register the timeouts
*/
#define SW_TIMER_MAX_SLOTS 16

#define SW_TIME_FREE_SLOT_FLAG 0xFFFFFFFFFFFFFFFFull

/*! \def SW_TIMER_HW_COUNT_PER_SEC
    \brief The clock speed of timer is 1MHz.
    NOTE: This 1Mhz is silicon timer clock. Other platforms like Zebu could have different clock.
*/
#define SW_TIMER_HW_COUNT_PER_SEC 1000000U

/*! \def SW_TIMER_SW_TICKS_TO_HW_COUNT(SW_TICKS)
    \brief Compute number of HW count in SW_TICKS
*/
#define SW_TIMER_SW_TICKS_TO_HW_COUNT(SW_TICKS) (SW_TICKS * SW_TIMER_HW_COUNT_PER_SEC)

/*! \def SW_TIMER_HW_COUNT_TO_SW_TICKS(HW_COUNT)
    \brief Coumpute SW ticks in HW_COUNT
*/
#define SW_TIMER_HW_COUNT_TO_SW_TICKS(HW_COUNT) (HW_COUNT / SW_TIMER_HW_COUNT_PER_SEC)

/*! \fn int32_t SW_Timer_Init(void)
    \brief Initializes the SW Timer
    \return Status indicating success or negative error
*/
int32_t SW_Timer_Init(void);

/*! \fn int32_t SW_Timer_Create_Timeout(void (*timeout_callback_fn)(uint8_t),
                                uint8_t callback_arg, uint32_t sw_ticks)
    \brief Adds new timeout entry for incoming command
    \param timeout_callback_fn Callback to be triggerd at timeout
    \param callback_arg argument for timeout callback
    \param sw_ticks SW ticks for timeout the cammand
    \return SW Timer slot used to register the tiemout or Error code in case of failure.
*/
int32_t SW_Timer_Create_Timeout(
    void (*timeout_callback_fn)(uint8_t), uint8_t callback_arg, uint32_t sw_ticks);

/*! \fn void SW_Timer_Cancel_Timeout(uint8_t sw_timer_idx)
    \brief Cancels the timeout already registered
    \param sw_timer_idx SW Time Slot number to cancel
    \return none
*/
void SW_Timer_Cancel_Timeout(uint8_t sw_timer_idx);

/*! \fn uint32_t SW_Timer_Get_Elapsed_Time(void)
    \brief Returns the elapsed time from last
           periodic timer update
    \return Residue delta time
*/
uint32_t SW_Timer_Get_Elapsed_Time(void);

/*! \fn void SW_Timer_Processing(void)
    \brief Function to handle periodic PU Timer triggers
    and check the timeout for added timers so far
*/
void SW_Timer_Processing(void);

/*! \fn bool SW_Timer_Interrupt_Status(void)
    \brief Get the status of the SW Timer Flag
    \return status of the SW Timer Int Flag
*/
_Bool SW_Timer_Interrupt_Status(void);

#endif /* SW_TIMER_H */
