/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------
 */
#include <inttypes.h>
#include <stdio.h>
#include "log.h"
#include "dm_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "dm_event_control.h"
#include "watchdog_task.h"

/* The variable used to hold task's handle */
static TaskHandle_t t_handle;

/* Task entry function */
static void watchdog_task_entry(void *pvParameters);

/* Task stack memory */
static StackType_t g_dm_stack[WDOG_TASK_STACK_SIZE];

/* The variable used to hold the task's data structure. */
static StaticTask_t g_staticTask_ptr;

/* Watchdog task entry function */
static void watchdog_task_entry(void *pvParameter);

int32_t init_watchdog_service(void)
{
    int32_t status = 0;

    /* Build time argument to configure Time slicing schedule delays. */
    status = watchdog_init(WDOG_DEFAULT_TIMEOUT_MSEC);
    if (!status)
    {
        /* Create the watchdog feeding task */
        t_handle = xTaskCreateStatic(watchdog_task_entry, "WDOG_TASK", WDOG_TASK_STACK_SIZE, NULL,
                                     WDOG_TASK_PRIORITY, g_dm_stack, &g_staticTask_ptr);
        if (!t_handle)
        {
            Log_Write(LOG_LEVEL_ERROR,
                      "Task Creation Failed: Failed to create Watchdog Handler Task.\n");
            status = -1;
        }
        else
        {
            Log_Write(LOG_LEVEL_INFO, "Watchdog service initialized.\n");
        }
    }

    return status;
}

static void watchdog_task_entry(void *pvParameter)
{
    (void)pvParameter;
    uint32_t watchdog_timeout;

    get_watchdog_timeout(&watchdog_timeout);
    /* Use 80% of the given time for the task delay
       to account for scheduling delays. Its a guess,
       might need to change*/
    const TickType_t frequency = pdMS_TO_TICKS((watchdog_timeout * 80) / 100);

    /* obtain reset cause form PMIC to determine if it was
       watchdog reset */
    uint32_t reset_cause = 0;

    if (0 != pmic_get_reset_cause(&reset_cause))
    {
        Log_Write(LOG_LEVEL_ERROR, "watchdog_task_entry: Cannot retrieve reset cause from PMIC.\n");
    }
    else
    {
        /* if it was a watchdog reset then inform host using watchdog event */
        if (reset_cause == PMIC_I2C_RESET_RESET_CAUSE_CRU_SYS_RESET)
        {
            struct event_message_t message;
            /* add details in message header and fill payload */
            FILL_EVENT_HEADER(&message.header, SP_WATCHDOG_RESET, sizeof(struct event_message_t))
            FILL_EVENT_PAYLOAD(&message.payload, WARNING, 0, timer_get_ticks_count(), reset_cause)
            wdog_timeout_callback(CORRECTABLE, &message);
        }
    }

    /* Initialise the xLastWakeTime variable with the current time. */
    TickType_t last_wake_time = xTaskGetTickCount();

    while (1)
    {
        vTaskDelayUntil(&last_wake_time, frequency);
        watchdog_kick();
    }
}
