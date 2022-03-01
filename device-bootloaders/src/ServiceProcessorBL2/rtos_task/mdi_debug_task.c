/*-------------------------------------------------------------------------
* Copyright (C) 2021, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/
/*! \file mdi_debug_task.c
    \brief A C module that creates a task for MDI BP event handling

    Public interfaces:
    create_dm_mdi_bp_notify_task
*/
/***********************************************************************/

#include "mdi_debug_task.h"

static TaskHandle_t g_dm_mdi_bp_notify_handle;
static StackType_t g_dm_mdi_bp_notify_task_stack[VQUEUE_STACK_SIZE];
static StaticTask_t g_dm_mdi_bp_notify_task_ptr;

__attribute__((noreturn)) static void dm_mdi_bp_notify_task(void *pvParameters);

/* The variable used to hold queue's handle */
QueueHandle_t q_dm_mdi_bp_notify_handle;

/* The variable used to hold the queue's data structure. */
static StaticQueue_t g_dm_mdi_staticQueue;

/* Queue storage */
uint8_t DM_MDI_Queue_Storage[DM_MDI_EVENT_QUEUE_SIZE];

void create_dm_mdi_bp_notify_task(void)
{
    q_dm_mdi_bp_notify_handle = xQueueCreateStatic(DM_MDI_EVENT_QUEUE_LENGTH,
                                                   DM_MDI_EVENT_QUEUE_ITEM_SIZE,
                                                   DM_MDI_Queue_Storage, &g_dm_mdi_staticQueue);

    if (!q_dm_mdi_bp_notify_handle)
    {
        Log_Write(LOG_LEVEL_ERROR,
                  "Message Queue creation error: Failed to create DM MDI Events Queue.\n");
    }

    /* Create MDI BP notify task */
    g_dm_mdi_bp_notify_handle = xTaskCreateStatic(dm_mdi_bp_notify_task, "DM_MDI_BP_Notify_Task",
                                                  VQUEUE_STACK_SIZE, NULL, VQUEUE_TASK_PRIORITY,
                                                  g_dm_mdi_bp_notify_task_stack,
                                                  &g_dm_mdi_bp_notify_task_ptr);
    if (g_dm_mdi_bp_notify_handle == NULL)
    {
        Log_Write(LOG_LEVEL_ERROR, "xTaskCreateStatic(create_dm_mdi_bp_notify_task) failed!\r\n");
        vQueueDelete(q_dm_mdi_bp_notify_handle);
    }
}

__attribute__((noreturn)) static void dm_mdi_bp_notify_task(void *pvParameters)
{
    struct device_mgmt_mdi_bp_event_t event;
    int status;
    bool ret;
    (void)pvParameters;

    while (1)
    {
        /* block until a message is received */
        if (xQueueReceive(q_dm_mdi_bp_notify_handle, &event, portMAX_DELAY) == pdTRUE)
        {
            Log_Write(LOG_LEVEL_INFO, "DM MDI BP set event received: %s\n", __func__);

            /* Wait for core to halt */
            ret = WAIT(check_halted());

            if (ret)
            {
                /* Breakpoint halt success */
                event.event_type = MDI_EVENT_TYPE_BP_HALT_SUCCESS;
            }
            else
            {
                /* Breakpoint halt failed. */
                event.event_type = MDI_EVENT_TYPE_BP_HALT_FAILED;
            }

            event.event_info.event_hdr.tag_id = 0xffff; /* Async Event Tag ID. */
            event.event_info.event_hdr.size = sizeof(event) - sizeof(struct cmn_header_t);
            event.event_info.event_hdr.msg_id = DM_CMD_MDI_SET_BREAKPOINT;
            status = SP_Host_Iface_CQ_Push_Cmd((void *)&event, sizeof(event));
            if (status)
            {
                Log_Write(LOG_LEVEL_ERROR, "dm_mdi_bp_notify_task :  push to CQ failed!\n");
            }
        }
    }
}
