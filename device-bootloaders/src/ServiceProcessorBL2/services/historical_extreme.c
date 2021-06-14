/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file historical_extreme.c
    \brief A C module that implements the Historical extreme value services

    Public interfaces:
        historical_extreme_value_request
*/
/***********************************************************************/

#include "bl2_historical_extreme.h"

/************************************************************************
*
*   FUNCTION
*
*       get_max_memory_error
*
*   DESCRIPTION
*
*       This function gets the maximum count of memory error.
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the
*                         Command Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void get_max_memory_error(tag_id_t tag_id, uint64_t req_start_time)
{
    struct device_mgmt_max_memory_error_rsp_t dm_rsp;
    int32_t status;
    uint32_t ddr_ce_count;
    volatile struct max_error_count_t *error_count = get_soc_max_control_block();

    Log_Write(LOG_LEVEL_INFO, "Historical extreme value request: %s\n",__func__);

    status = ddr_get_ce_count(&ddr_ce_count);
    if (0 != status) {
        Log_Write(LOG_LEVEL_ERROR, "get_max_memory_error : driver error !\n");
    } else {
        if (error_count->ddr_ce_max_count < ddr_ce_count) {
            error_count->ddr_ce_max_count = ddr_ce_count;
        }

        dm_rsp.max_ecc_count.count = error_count->ddr_ce_max_count;
    }

    Log_Write(LOG_LEVEL_INFO, "Historical extreme value response: %s\n",__func__);

    FILL_RSP_HEADER(dm_rsp, tag_id, DM_CMD_GET_MAX_MEMORY_ERROR,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp,
                                       sizeof(struct device_mgmt_max_memory_error_rsp_t))) {
        Log_Write(LOG_LEVEL_ERROR, "get_max_memory_error: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_max_ddr_bw
*
*   DESCRIPTION
*
*       This function gets the maximum DDR BW.
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the
*                         Command Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void get_module_max_ddr_bw(tag_id_t tag_id, uint64_t req_start_time)
{
    struct device_mgmt_max_dram_bw_rsp_t dm_rsp;
    struct max_dram_bw_t max_dram_bw;
    int32_t status;

    Log_Write(LOG_LEVEL_INFO, "Historical extreme value request: %s\n",__func__);

    status = get_module_max_dram_bw(&max_dram_bw);

    if (0 != status) {
        Log_Write(LOG_LEVEL_ERROR, " perf mgmt svc error: get_module_max_dram_bw()\r\n");
    } else {
        dm_rsp.max_dram_bw.max_bw_rd_req_sec = max_dram_bw.max_bw_rd_req_sec;
        dm_rsp.max_dram_bw.max_bw_wr_req_sec = max_dram_bw.max_bw_wr_req_sec;
    }

    Log_Write(LOG_LEVEL_INFO, "Historical extreme value response: %s\n",__func__);

    FILL_RSP_HEADER(dm_rsp, tag_id, DM_CMD_GET_MODULE_MAX_DDR_BW,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 !=
        SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_max_dram_bw_rsp_t))) {
        Log_Write(LOG_LEVEL_ERROR, "get_max_memory_error: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_max_throttle_time
*
*   DESCRIPTION
*
*       This function returns the maximum total time the device has been
*       resident in the throttles state i.e. non-full power mode from Device Reset.
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the
*                         Command Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void get_module_max_throttle_time(tag_id_t tag_id, uint64_t req_start_time)
{
    struct device_mgmt_max_throttle_time_rsp_t dm_rsp;
    uint64_t max_throttle_time;
    int32_t status;

    Log_Write(LOG_LEVEL_INFO, "Historical extreme value request: %s\n",__func__);

    status = get_max_throttle_time(&max_throttle_time);

    if (0 != status) {
        Log_Write(LOG_LEVEL_ERROR, " thermal pwr mgmt svc error: get_module_max_throttle_time()\r\n");
    } else {
        dm_rsp.max_throttle_time.time_usec = max_throttle_time;
    }

    Log_Write(LOG_LEVEL_INFO, "Historical extreme value response: %s\n",__func__);

    FILL_RSP_HEADER(dm_rsp, tag_id, DM_CMD_GET_MODULE_MAX_THROTTLE_TIME,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp,
                                       sizeof(struct device_mgmt_max_throttle_time_rsp_t))) {
        Log_Write(LOG_LEVEL_ERROR, "get_module_max_throttle_time: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_max_temperature
*
*   DESCRIPTION
*
*       This function returns the historical_extreme Maximum Device temperature
*       as sample periodically from the PMIC
*       Note the value doesnt hold over a consecutive Device Reset
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void get_module_max_temperature(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_max_temperature_rsp_t dm_rsp;
    uint8_t max_temp;
    int32_t status;

    Log_Write(LOG_LEVEL_INFO, "Historical extreme value request: %s\n",__func__);

    status = get_soc_max_temperature(&max_temp);

    if (0 != status) {
        Log_Write(LOG_LEVEL_ERROR, " thermal pwr mgmt svc error: get_soc_max_temperature()\r\n");
    } else {
        dm_rsp.max_temperature.max_temperature_c = max_temp;
    }

    Log_Write(LOG_LEVEL_INFO, "Historical extreme value response: %s\n",__func__);

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_MODULE_MAX_TEMPERATURE,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp,
                                       sizeof(struct device_mgmt_max_temperature_rsp_t))) {
        Log_Write(LOG_LEVEL_ERROR, "get_module_max_temperature: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       historical_extreme_value_request
*
*   DESCRIPTION
*
*       This function takes as input the command ID from Host,
*       and accordingly either calls the respective error control info
*       functions
*
*   INPUTS
*
*       msg_id      Unique enum representing specific command
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void historical_extreme_value_request(tag_id_t tag_id, msg_id_t msg_id)
{
    uint64_t req_start_time;

    req_start_time = timer_get_ticks_count();

    switch (msg_id) {
    case DM_CMD_GET_MAX_MEMORY_ERROR: {
        get_max_memory_error(tag_id, req_start_time);
    } break;
    case DM_CMD_GET_MODULE_MAX_DDR_BW: {
        get_module_max_ddr_bw(tag_id, req_start_time);
    } break;
    case DM_CMD_GET_MODULE_MAX_THROTTLE_TIME: {
        get_module_max_throttle_time(tag_id, req_start_time);
    } break;
    case DM_CMD_GET_MODULE_MAX_TEMPERATURE: {
        get_module_max_temperature(tag_id, req_start_time);
    } break;
    }
}
