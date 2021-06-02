/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
************************************************************************
************************************************************************
*
*   DESCRIPTION
*
*       This file implements the DM sampling task for sampling different
*       thermal and performance Management parameters.
*
*   FUNCTIONS
*
*       - init_dm_sampling_task
*
***********************************************************************/
#include <inttypes.h>
#include <stdio.h>
#include "log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bl2_pmic_controller.h"
#include "dm.h"
#include "dm_task.h"
#include "perf_mgmt.h"
#include "thermal_pwr_mgmt.h"

// TODO: will be configurable by the host
#define DM_TASK_DELAY_MS 100

#define DM_TASK_STACK    1024
#define DM_TASK_PRIORITY 1

/* GLobals */
static TaskHandle_t g_dm_task_handle;
static StackType_t g_dm_stack[DM_TASK_STACK];
static StaticTask_t g_staticTask_ptr;

/* Task entry function */
static void dm_task_entry(void *pvParameters);

/************************************************************************
*
*   FUNCTION
*
*       init_dm_sampling_task
*
*   DESCRIPTION
*
*       This function creates te DM sampling task.
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
void init_dm_sampling_task(void)
{
    g_dm_task_handle = xTaskCreateStatic(dm_task_entry, "DM_TASK", DM_TASK_STACK, NULL,
                                         DM_TASK_PRIORITY, g_dm_stack, &g_staticTask_ptr);

    // Initialize the globals

    //g_soc_perf_reg.xyz  = <value>;
    //g_soc_power_reg.xyz = <value>;

    if (!g_dm_task_handle) {
        Log_Write(LOG_LEVEL_ERROR, "Task creation error: Failed to create DM sampling task.\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       dm_task_entry
*
*   DESCRIPTION
*
*       This DM sampling task entry point. The DM sampling task
*       periodically samples different thermal and perf parameters
*       and save them in globals which are used by the respective
*       request handlers.
*
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
static void dm_task_entry(void *pvParameters)
{
    (void)pvParameters;
    int ret;

    //Will need to cleanly yield this thread to avoid this Thread from hoging the SP
    while (1) {
        // simulate the values fetched from the PMIC Interface
        // Module Temperature in C
        ret = update_module_current_temperature();

        if (0 != ret) {
            Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error : update_module_current_temperature()\r\n");
        }

        // Module Power in watts
        ret = update_module_soc_power();

        if (0 != ret) {
            Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error : get_module_soc_power()\r\n");
        }

        // Update module max temperature
        update_module_max_temp();

        // Update the module uptime
        update_module_uptime();

        // Update the DRAM BW(Read/Write request) details
        ret = update_dram_bw();

        if (0 != ret) {
            Log_Write(LOG_LEVEL_ERROR, "perf mgmt svc error : update_dram_bw()\r\n");
        }

        // DRAM capacity
        ret = update_dram_capacity_percent();
        if (0 != ret) {
            Log_Write(LOG_LEVEL_ERROR, "perf mgmt svc error : update_dram_capacity_percent()\r\n");
        }

        // Wait for the sampling period
        // Need to implement Timer Watchdog based interrupt
        vTaskDelay(DM_TASK_DELAY_MS);
    }
}
