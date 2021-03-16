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

int32_t init_watchdog_service(uint32_t timeout_msec) {

    int32_t status = -1;

    if (!timeout_msec) {
        return status;
    }

    /* Init watch dog timer here */
    status = watchdog_init(timeout_msec);
    if (!status) {
        /* Create the watchdog feeding task */
        t_handle = xTaskCreateStatic(watchdog_task_entry, "WDOG_TASK", WDOG_TASK_STACK_SIZE,
                        &timeout_msec, WDOG_TASK_PRIORITY, g_dm_stack, &g_staticTask_ptr);
        if (!t_handle) {
            printf("Task Creation Failed: Failed to create Watchdog Handler Task.\n");
            status = -1;
        }
    }

    return status;
}

static void watchdog_task_entry(void *pvParameter) {

    uint32_t delay = *((uint32_t*) pvParameter);

    while (1) {
        /* Feed the watch dog */
        watchdog_kick();

        /* Use 80% of the given time for the task delay
         to account for scheduling delays. Purely a guess
         might need to change*/

        vTaskDelay((delay * 80) / 100);
    }
}
