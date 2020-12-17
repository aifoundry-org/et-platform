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
*       This file implements the following service functions.
*
*   FUNCTIONS
*
*       Host Requested Services functions:
*       
*       - historical_extreme_value_request
*       - get_max_memory_error
*       - get_module_max_ddr_bw
*       - get_module_max_throttle_time
*
***********************************************************************/

#include "bl2_link_mgmt.h"
#include "bl2_historical_extreme.h"
#include "dm.h"

static uint8_t cqueue_push(char *buf, uint32_t size)
{
    printf("Pointer to buf %s with payload size: %d\n", buf, size);
    return 0;
}

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
static void get_max_memory_error(uint64_t req_start_time)
{
    struct max_memory_error_rsp_t dm_rsp;
    dm_rsp.rsp_hdr.status = DM_STATUS_SUCCESS;
    dm_rsp.rsp_hdr.size = sizeof(dm_rsp.max_ecc_count);

    //TODO : This should be retrieved from BL2 Error data struct.
    dm_rsp.max_ecc_count = 5;

    dm_rsp.rsp_hdr.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer;
    memcpy(p, &dm_rsp, sizeof(dm_rsp));

    if (0 != cqueue_push(p, sizeof(dm_rsp))) {
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
static void get_module_max_ddr_bw(uint64_t req_start_time)
{
    struct max_dram_bw_rsp_t dm_rsp;
    dm_rsp.rsp_hdr.status = DM_STATUS_SUCCESS;
    dm_rsp.rsp_hdr.size = sizeof(dm_rsp.max_bw_rd_req_sec) + sizeof(dm_rsp.max_bw_wr_req_sec);

    //TODO : These should be retrieved from BL2 PMC data structure.
    dm_rsp.max_bw_rd_req_sec = 100;
    dm_rsp.max_bw_wr_req_sec = 100;

    dm_rsp.rsp_hdr.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer;
    memcpy(p, &dm_rsp, sizeof(dm_rsp));

    if (0 != cqueue_push(p, sizeof(dm_rsp))) {
        printf("get_module_max_ddr_bw: Cqueue push error !\n");
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
static void get_module_max_throttle_time(uint64_t req_start_time)
{
    struct max_throttle_time_rsp_t dm_rsp;
    dm_rsp.rsp_hdr.status = DM_STATUS_SUCCESS;
    dm_rsp.rsp_hdr.size = sizeof(dm_rsp.max_time_usec);

    //TODO : This should be retrieved from BL2 PMC data structure.
    dm_rsp.max_time_usec = 1000;

    dm_rsp.rsp_hdr.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer;
    memcpy(p, &dm_rsp, sizeof(dm_rsp));

    if (0 != cqueue_push(p, sizeof(dm_rsp))) {
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
*       cmd_id      Unique enum representing specific command   
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void historical_extreme_value_request(uint32_t cmd_id)
{
    uint64_t req_start_time;

    req_start_time = timer_get_ticks_count();

    switch (cmd_id) {
    case GET_MAX_MEMORY_ERROR: {
        get_max_memory_error(req_start_time);
    } break;
    case GET_MODULE_MAX_DDR_BW: {
        get_module_max_ddr_bw(req_start_time);
    } break;
    case GET_MODULE_MAX_THROTTLE_TIME: {
        get_module_max_throttle_time(req_start_time);
    } break;
    }
}