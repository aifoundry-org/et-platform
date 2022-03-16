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

static void dm_mdi_bp_notify_host(bool bp_hit)
{

    int status;
    struct device_mgmt_mdi_bp_event_t event;

    if(bp_hit)
    {
        Log_Write(LOG_LEVEL_INFO, "MDI_EVENT_TYPE_BP_HALT_SUCCESS: %s\n", __func__);
        event.event_type = MDI_EVENT_TYPE_BP_HALT_SUCCESS;
    } 
    else
    {
        Log_Write(LOG_LEVEL_INFO, "MDI_EVENT_TYPE_BP_HALT_FAILURE: %s\n", __func__);
        event.event_type = MDI_EVENT_TYPE_BP_HALT_FAILED;
    }
    
    event.event_info.event_hdr.tag_id = 0xffff; /* Async Event Tag ID. */
    event.event_info.event_hdr.size = sizeof(event) - sizeof(struct cmn_header_t);
    event.event_info.event_hdr.msg_id = DM_CMD_MDI_SET_BREAKPOINT_EVENT;
    status = SP_Host_Iface_CQ_Push_Cmd((void *)&event, sizeof(event));
    if (status)
    {
        Log_Write(LOG_LEVEL_ERROR, "dm_mdi_bp_notify_host:  push to CQ failed!\n");
    }
}

__attribute__((noreturn)) static void dm_mdi_bp_notify_task(void *pvParameters)
{
    bool ret = false;
    uint64_t bp_timeout;
    (void)pvParameters;
    struct mdi_bp_control_cmd_t mdi_cmd_req;

    while (1)
    {
        /* block until a message is received */
        if (xQueueReceive(q_dm_mdi_bp_notify_handle, &mdi_cmd_req, portMAX_DELAY) == pdTRUE)
        {
            Log_Write(LOG_LEVEL_INFO, "DM MDI BP set event received: %s\n", __func__);
            bp_timeout =  mdi_cmd_req.cmd_attr.bp_event_wait_timeout;

            while ((bp_timeout > 0) && (!ret))
            {
                /* Wait for the BP timeout period specified */
                vTaskDelay(pdMS_TO_TICKS(DM_MDI_BP_NOTIFY_TASK_DELAY_MS));

                bp_timeout -= DM_MDI_BP_NOTIFY_TASK_DELAY_MS;

                /* Check if the core is halted due to BP */
                ret = HART_HALT_STATUS(mdi_cmd_req.cmd_attr.hart_id);

            }

            if (ret)
            {
                /* Breakpoint halt success */
                Log_Write(LOG_LEVEL_INFO, "Hart Halted due to breakpoint\n");
                dm_mdi_bp_notify_host(true);
            }
            else
            {
                /* Breakpoint halt did not occur within specified timeout */
               Log_Write(LOG_LEVEL_INFO, "Hart failed to Halt within user specified timeout\r\n"); 
               dm_mdi_bp_notify_host(false);    
            }
 
            /* Re-initialize the variable  */
            ret = false;
        }
    }
}
