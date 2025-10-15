/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

#include <stdint.h>
#include "delays.h"
#include "interrupt.h"

#include "FreeRTOS.h"
#include "task.h"

#include "bl2_timer.h"

/*
** Implementation for us delay
** This functions waits for RVTimer in a CPU tight loop
*/
void usdelay(uint32_t usec)
{
    uint64_t target_tick;

    if (usec == 0)
        return;

    target_tick = timer_get_ticks_count() + usec;
    while (timer_get_ticks_count() < target_tick)
        ;
}

/*
** Implementation for ms delay
** This function uses
**   vTaskDelay() after FreeRTOS scheduler is up (task yield)
**   usdelay() before FreeRTOS scheduler is up (cpu spin-loop)
*/
void msdelay(uint32_t msec)
{
    if (msec == 0)
        return;

    if (!INT_Is_Trap_Context() && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED))
        vTaskDelay(msec / portTICK_PERIOD_MS);
    else
        usdelay(msec * 1000);
}
