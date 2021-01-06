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

#include "dm.h"
#include "dm_service.h"
#include "sp_host_iface.h"
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

    //TODO : This should be retrieved from BL2 Error data struct.
    dm_rsp.max_ecc_count.count = 5;

    FILL_RSP_HEADER(dm_rsp, tag_id,
                    DM_CMD_GET_MAX_MEMORY_ERROR,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_max_memory_error_rsp_t))) {
        printf("get_max_memory_error: Cqueue push error !\n");
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

    //TODO : These should be retrieved from BL2 PMC data structure.
    dm_rsp.max_dram_bw.max_bw_rd_req_sec = 100;
    dm_rsp.max_dram_bw.max_bw_wr_req_sec = 100;

    FILL_RSP_HEADER(dm_rsp, tag_id,
                    DM_CMD_GET_MAX_MEMORY_ERROR,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_max_dram_bw_rsp_t))) {
        printf("get_max_memory_error: Cqueue push error !\n");
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

    //TODO : This should be retrieved from BL2 PMC data structure.
    dm_rsp.max_throttle_time.time_usec = 1000;

    FILL_RSP_HEADER(dm_rsp, tag_id,
                    DM_CMD_GET_MODULE_MAX_THROTTLE_TIME,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);


    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_max_throttle_time_rsp_t))) {
        printf("get_module_max_throttle_time: Cqueue push error !\n");
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
    }
}
