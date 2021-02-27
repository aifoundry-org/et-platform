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

    status = ddr_get_ce_count(&ddr_ce_count);
    if (!status) {
        if (error_count->ddr_ce_max_count < ddr_ce_count) {
            error_count-> ddr_ce_max_count = ddr_ce_count;
        }
        // TODO: Change this to uint32_t in rsp control block. same value is present in the json schema as well sw-6467
        dm_rsp.max_ecc_count.count = (uint8_t)error_count->ddr_ce_max_count; 
    }

    if (status) {
        printf("get_max_memory_error : driver error !\n");
    }

    FILL_RSP_HEADER(dm_rsp, tag_id,
                    DM_CMD_GET_MAX_MEMORY_ERROR,
                    timer_get_ticks_count() - req_start_time,
                    (uint32_t)status); // TODO update the status type - SW-6467
 

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_max_memory_error_rsp_t))) {
        printf("get_max_memory_error: Cqueue push error!\n");
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

    max_dram_bw = get_module_max_dram_bw_gbl();

    dm_rsp.max_dram_bw.max_bw_rd_req_sec = max_dram_bw.max_bw_rd_req_sec;
    dm_rsp.max_dram_bw.max_bw_wr_req_sec = max_dram_bw.max_bw_wr_req_sec;

    FILL_RSP_HEADER(dm_rsp, tag_id,
                    DM_CMD_GET_MODULE_MAX_DDR_BW,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_max_dram_bw_rsp_t))) {
        printf("get_max_memory_error: Cqueue push error!\n");
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

    dm_rsp.max_throttle_time.time_usec = get_module_max_throttle_time_gbl();

    FILL_RSP_HEADER(dm_rsp, tag_id,
                    DM_CMD_GET_MODULE_MAX_THROTTLE_TIME,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);


    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_max_throttle_time_rsp_t))) {
        printf("get_module_max_throttle_time: Cqueue push error!\n");
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

    dm_rsp.max_temperature.max_temperature_c = get_module_max_temperature_gbl();

    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_MODULE_MAX_TEMPERATURE,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);


   if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_max_temperature_rsp_t))) {
        printf("get_module_max_temperature: Cqueue push error!\n");
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
