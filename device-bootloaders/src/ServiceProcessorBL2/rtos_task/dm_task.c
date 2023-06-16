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
#include "config/mgmt_build_config.h"
#include "bl2_pmic_controller.h"
#include "mem_controller.h"
#include "bl2_cache_control.h"
#include "pcie_configuration.h"
#include "dm.h"
#include "dm_task.h"
#include "perf_mgmt.h"
#include "thermal_pwr_mgmt.h"

/* GLobals */
/* DM Task */
static TaskHandle_t g_dm_task_handle;
static StackType_t g_dm_stack[DM_TASK_STACK];
static StaticTask_t g_staticTask_ptr;

/* Task entry functions */
static void dm_task_entry(void *pvParameters);

/* Functions to log dev stats */
static void dm_log_operating_point_stats(void);

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

    if (!g_dm_task_handle)
    {
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

    ret = Thermal_Pwr_Mgmt_Init_OP_Stats();
    if (ret != STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR, "Error initializing OP stats: %s\n", __func__);
    }
    while (1)
    {
        Log_Write(LOG_LEVEL_DEBUG, "Updating the periodically sampled parameters: %s\n", __func__);

        ret = update_module_current_temperature();

        if (0 != ret)
        {
            Log_Write(LOG_LEVEL_ERROR,
                      "thermal pwr mgmt svc error : update_module_current_temperature()\r\n");
        }

        // update PMB stats, it updates voltage, current and power for minion, NOC and SRAM modules
        ret = update_pmb_stats(false);

        if (0 != ret)
        {
            Log_Write(LOG_LEVEL_ERROR, "thermal pwr mgmt svc error : update_pmb_stats()\r\n");
        }

        // Module Power in watts
        ret = update_module_soc_power();

        if (0 != ret)
        {
            Log_Write(LOG_LEVEL_ERROR,
                      "thermal pwr mgmt svc error : update_module_soc_power()\r\n");
        }

        // Module frequencies
        ret = update_module_frequencies();

        if (0 != ret)
        {
            Log_Write(LOG_LEVEL_ERROR,
                      "thermal pwr mgmt svc error : update_module_frequencies()\r\n");
        }

        /*  Update the module uptime */
        ret = update_module_uptime();

        if (0 != ret)
        {
            Log_Write(LOG_LEVEL_ERROR, "update_module_uptime error : update_module_uptime()\r\n");
        }

        /* Update MM stats */
        ret = update_mm_stats();
        if (0 != ret)
        {
            Log_Write(LOG_LEVEL_ERROR, "perf mgmt svc error : update_mm_stats()\r\n");
        }

        /* Update the DRAM BW(Read/Write request) details */
        ret = update_dram_bw();

        if (0 != ret)
        {
            Log_Write(LOG_LEVEL_ERROR, "perf mgmt svc error : update_dram_bw()\r\n");
        }

        /* DRAM capacity */
        ret = update_dram_capacity_percent();
        if (0 != ret)
        {
            Log_Write(LOG_LEVEL_ERROR, "perf mgmt svc error : update_dram_capacity_percent()\r\n");
        }

        /* Log op stats to trace */
        dm_log_operating_point_stats();

        /* Wait for the sampling period */
        vTaskDelay(pdMS_TO_TICKS(DM_TASK_DELAY_MS));
    }
}

/************************************************************************
*
*   FUNCTION
*
*       dm_log_operating_point_stats
*
*   DESCRIPTION
*
*       This function will collect sample of current values of temperature and power
*       and calculate min, max and moving average.
*
*
*   INPUTS
*
*       count samples counter
*       op_stats operating point stats structure
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void dm_log_operating_point_stats(void)
{
    struct op_stats_t op_stats = { 0 };

    if (SUCCESS == Thermal_Pwr_Mgmt_Get_System_Power_Temp_Stats(&op_stats))
    {
        /* Dump the data to trace using SP custom event */
        Trace_Custom_Event(Trace_Get_Dev_Stats_CB(), TRACE_CUSTOM_TYPE_SP_OP_STATS,
                           (uint8_t *)&op_stats, sizeof(struct op_stats_t));

        /* Update data size in stats buffer */
        Trace_Update_SP_Stats_Buffer_Header();
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "perf mgmt svc error : unable to get op stats\r\n");
    }
}
