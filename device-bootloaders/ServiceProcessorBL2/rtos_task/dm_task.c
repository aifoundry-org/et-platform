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
#include "perf_mgmt.h"
#include "thermal_pwr_mgmt.h"

// TODO: will be configurable by the host
#define DM_TASK_DELAY_MS   5

#define DM_TASK_STACK      1024
#define DM_TASK_PRIORITY   1

#define HOURS_IN_DAY       24
#define SECONDS_IN_HOUR    3600
#define SECONDS_IN_MINUTE  60 

/* GLobals */
static TaskHandle_t g_dm_task_handle;
static StackType_t g_dm_stack[DM_TASK_STACK];
static StaticTask_t g_staticTask_ptr;

/* Task entry function */
static void dm_task_entry(void *pvParameters);

struct soc_perf_reg_t  g_soc_perf_reg __attribute__((section(".data")));

volatile struct soc_perf_reg_t *get_soc_perf_reg(void)
{
    return &g_soc_perf_reg;
}

struct soc_power_reg_t g_soc_power_reg __attribute__((section(".data")));

volatile struct soc_power_reg_t *get_soc_power_reg(void)
{
    return &g_soc_power_reg;
}

static void get_max_temperature(void) {
   uint8_t curr_temp;

   curr_temp = pmic_get_temperature();

   if (get_module_max_temperature_gbl() < curr_temp) {
        update_module_max_temp_gbl(curr_temp);
   }
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
    uint64_t module_uptime;
    uint64_t seconds;
    uint16_t day;
    uint8_t hours;
    uint8_t minutes;
    struct max_dram_bw_t max_dram_bw;
    struct dram_bw_t dram_bw;
    int ret;

    //Will need to cleanly yield this thread to avoid this Thread from hoging the SP
    while(1)  
    {
        // simulate the values fetched from the PMIC Interface
        // Module Temperature in C
        get_soc_power_reg()->soc_temperature = pmic_get_temperature();
        // Module Power in watts
        get_soc_power_reg()->soc_power = pmic_read_soc_power();

        // Get module max temperature
        get_max_temperature();
        
        // Module Uptime //
        module_uptime = timer_get_ticks_count();

        seconds = (module_uptime / 1000000);
        day = (uint16_t) (seconds / (HOURS_IN_DAY * SECONDS_IN_HOUR)); 
        seconds = (seconds % (HOURS_IN_DAY * SECONDS_IN_HOUR)); 
        hours = (uint8_t)(seconds / SECONDS_IN_HOUR); 

        seconds %= SECONDS_IN_HOUR; 
        minutes = (uint8_t)(seconds / SECONDS_IN_MINUTE); 

        get_soc_power_reg()->module_uptime.day = day; //days
        get_soc_power_reg()->module_uptime.hours = hours; //hours
        get_soc_power_reg()->module_uptime.mins = minutes; //mins;

        ret = get_dram_bw(&dram_bw);

        if(!ret)
        {
            // DRAM BW
            get_soc_perf_reg()->dram_bw.read_req_sec = dram_bw.read_req_sec;
            get_soc_perf_reg()->dram_bw.write_req_sec = dram_bw.write_req_sec;
        }

        ret = get_max_dram_bw(&max_dram_bw);

        if(!ret)
        {
            // MAX DRAM BW
            get_soc_perf_reg()->max_dram_bw.max_bw_rd_req_sec = max_dram_bw.max_bw_rd_req_sec;
            get_soc_perf_reg()->max_dram_bw.max_bw_wr_req_sec =  max_dram_bw.max_bw_wr_req_sec;
        }

        update_module_max_throttle_time_gbl();
        
        // DRAM capacity //
        get_soc_perf_reg()->dram_capacity_percent = 80;

        // Wait for the sampling period //
        // Need to implement Timer Watchdog based interrupt
        vTaskDelay(DM_TASK_DELAY_MS);
    }
}