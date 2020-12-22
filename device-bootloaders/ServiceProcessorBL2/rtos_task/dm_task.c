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
#include "FreeRTOS.h"
#include "task.h"
#include "bl2_pmic_controller.h"
#include "dm.h"
#include "dm_service.h"
#include "dm_task.h"

// TODO: will be configurable by the host
static const TickType_t xDelay_msec = 100;

#define DM_TASK_STACK      1024
#define DM_TASK_PRIORITY   1

/* GLobals */
static TaskHandle_t g_dm_task_handle;
static StackType_t g_dm_stack[DM_TASK_STACK];
static StaticTask_t g_staticTask_ptr;

/* Task entry function */
static void dm_task_entry(void *pvParameters);

struct soc_perf_reg_t  *g_soc_perf_reg __attribute__((section(".data")));

volatile struct soc_perf_reg_t *get_soc_perf_reg(void)
{
    return g_soc_perf_reg;
}

struct soc_power_reg_t *g_soc_power_reg __attribute__((section(".data")));

volatile struct soc_power_reg_t *get_soc_power_reg(void)
{
    return g_soc_power_reg;
}

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
    g_dm_task_handle = xTaskCreateStatic(dm_task_entry, "DM_TASK", DM_TASK_STACK,
                                         NULL, DM_TASK_PRIORITY , g_dm_stack,
                                         &g_staticTask_ptr);

    // Initialize the globals

    //g_soc_perf_reg.xyz  = <value>;
    //g_soc_power_reg.xyz = <value>;

    if(!g_dm_task_handle)
    {
        printf("Task creation error: Failed to create DM sampling task.\n");
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
    //Will need to cleanly yield this thread to avoid this Thread from hoging the SP
    while(1)  
    {
        // simulate the values fetched from the PMIC Interface
        // TODO: Replace eventually with the actual values obtained from the hardware
        // Module Temperature in C
        get_soc_power_reg()->soc_temperature = pmic_get_temperature();
        // Module Power in watts//
        get_soc_power_reg()->soc_power = pmic_read_soc_power();
        
        // Module Uptime //
        // Subtract from the current     
        get_soc_power_reg()->module_uptime.day = 170; //days
        get_soc_power_reg()->module_uptime.hours = 10; //hours
        get_soc_power_reg()->module_uptime.mins = 20; //mins;
        // DRAM BW //
        get_soc_perf_reg()->dram_bw.read_req_sec = 100;
        get_soc_perf_reg()->dram_bw.write_req_sec = 100;
        // DRAM capacity //
        get_soc_perf_reg()->dram_capacity_percent = 80;
        // Wait for the sampling period //
        // Need to implement Timer Watchdog based interrupt
        vTaskDelay(xDelay_msec);
    }
}
