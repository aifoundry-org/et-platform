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
#include "sp_host_iface.h"
#include "bl2_pcie.h"
#include "bl2_sram.h"
#include "bl2_pmic_controller.h"
#include "bl2_ddr_init.h"
#include "bl2_watchdog.h"

/* The variable used to hold the queue's data structure. */
static StaticQueue_t g_staticQueue;

/* Queue storage */
uint8_t DM_Queue_Storage[DM_EVENT_QUEUE_SIZE];

/* The variable used to hold queue's handle */
static QueueHandle_t q_handle;

/* The variable used to hold task's handle */
static TaskHandle_t t_handle;

/* Task entry function */
static void dm_event_task_entry(void *pvParameters);

/* Task stack memory */
static StackType_t g_dm_stack[DM_EVENT_TASK_STACK];

/* The variable used to hold the task's data structure. */
static StaticTask_t g_staticTask_ptr;

/* Global control block for max error count */
struct max_error_count_t g_max_error_count __attribute__((section(".data")));

/* Callback prototypes */
static void pcie_event_callback(enum error_type type, struct event_message_t *msg);
static void sram_event_callback(enum error_type type, struct event_message_t *msg);
static void ddr_event_callback(enum error_type type, struct event_message_t *msg);
static void pmic_event_callback(enum error_type type, struct event_message_t *msg);
static void wdog_timeout_callback(enum error_type type, struct event_message_t *msg);

volatile struct max_error_count_t *get_soc_max_control_block(void)
{
    return &g_max_error_count;
}

int32_t dm_event_control_init(void)
{
    int32_t status = -1;

    q_handle = xQueueCreateStatic(DM_EVENT_QUEUE_LENGTH, DM_EVENT_QUEUE_ITEM_SIZE, DM_Queue_Storage,
                                  &g_staticQueue);

    if (!q_handle) {
        printf("Message Queue creation error: Failed to create DM Events Queue.\n");
        return status;
    }

    t_handle = xTaskCreateStatic(dm_event_task_entry, "DM_EVENT_TASK", DM_EVENT_TASK_STACK, NULL,
                                 DM_EVENT_TASK_PRIORITY, g_dm_stack, &g_staticTask_ptr);

    if (!t_handle) {
        printf("Task Creation Failed: Failed to create DM Event Handler Task.\n");
        vQueueDelete(q_handle);
        return status;
    }

    /* Init error control subsystem */
    status = pcie_error_control_init(pcie_event_callback);
    if (!status) {
        status = ddr_error_control_init(ddr_event_callback);
        if (!status) {
            status = sram_error_control_init(sram_event_callback);
            if (!status) {
                status = pmic_error_control_init(pmic_event_callback);
                if (!status) {
                    status = watchdog_error_init(wdog_timeout_callback);
                }
            }
        }
    }

    if (status) {
        printf("Error Control Init Failed: Failed to init error control\n");
        return status;
    }

    return status;
}

static void dm_event_task_entry(void *pvParameters)
{
    struct event_message_t msg;
    int status;

    (void)pvParameters;

    while (1) {
        /* block until a message is received */
        if (xQueueReceive(q_handle, &msg, portMAX_DELAY) == pdTRUE) {
            status = SP_Host_Iface_CQ_Push_Cmd((void *)&msg, sizeof(msg));
            if (status) {
                printf("dm_event_handler_task_error :  push to CQ failed!\n");
            }
        }
    }
}

static void pcie_event_callback(enum error_type type, struct event_message_t *msg)
{
    uint32_t error_count = EVENT_PAYLOAD_GET_ERROR_COUNT(&msg->payload);
    if (type == CORRECTABLE) {
        if (error_count > g_max_error_count.pcie_ce_max_count)
            g_max_error_count.pcie_ce_max_count = error_count;
    } else {
        if (error_count > g_max_error_count.pcie_uce_max_count)
            g_max_error_count.pcie_uce_max_count = error_count;
    }

    /* Post message to the queue */
    xQueueSendFromISR(q_handle, msg, (BaseType_t *)NULL);
}

static void sram_event_callback(enum error_type type, struct event_message_t *msg)
{
    uint32_t error_count = EVENT_PAYLOAD_GET_ERROR_COUNT(&msg->payload);

    if (type == CORRECTABLE) {
        if (error_count > g_max_error_count.sram_ce_max_count)
            g_max_error_count.pcie_ce_max_count = error_count;
    } else {
        if (error_count > g_max_error_count.sram_uce_max_count)
            g_max_error_count.pcie_uce_max_count = error_count;
    }

    /* Post message to the queue */
    xQueueSendFromISR(q_handle, msg, (BaseType_t *)NULL);
}

static void ddr_event_callback(enum error_type type, struct event_message_t *msg)
{
    uint32_t error_count = EVENT_PAYLOAD_GET_ERROR_COUNT(&msg->payload);

    if (type == CORRECTABLE) {
        if (error_count > g_max_error_count.ddr_ce_max_count)
            g_max_error_count.pcie_ce_max_count = error_count;
    } else {
        if (error_count > g_max_error_count.ddr_uce_max_count)
            g_max_error_count.pcie_uce_max_count = error_count;
    }

    /* Post message to the queue */
    xQueueSendFromISR(q_handle, msg, (BaseType_t *)NULL);
}

static void pmic_event_callback(enum error_type type, struct event_message_t *msg)
{
    (void)type;
    /* Post message to the queue */
    xQueueSendFromISR(q_handle, msg, (BaseType_t *)NULL);
}

static void wdog_timeout_callback(enum error_type type, struct event_message_t *msg)
{
    (void)type;
    /* Post message to the queue */
    xQueueSendFromISR(q_handle, msg, (BaseType_t *)NULL);
}
