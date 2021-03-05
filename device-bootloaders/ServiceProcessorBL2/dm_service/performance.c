/*************************************************************************
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
*************************************************************************/
/*! \file performance.c
    \brief A C module that implements the performance Management services

    Public interfaces:
        process_performance_request
*/
/***********************************************************************/

#include "bl2_perf.h"

/************************************************************************
*
*   FUNCTION
*
*       dm_svc_perf_get_asic_frequencies
*
*   DESCRIPTION
*
*       This function returns the frequencies of various clock domains.
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
static void dm_svc_perf_get_asic_frequencies(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_asic_frequencies_rsp_t dm_rsp;
    struct asic_frequencies_t asic_frequencies;
    int status;

    status  = get_module_asic_frequencies(&asic_frequencies);

    if(0 != status) {
        printf("perf mgmt error: get_module_asic_frequencies()\r\n");
    } else {
        dm_rsp.asic_frequency = asic_frequencies;
    }

    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_ASIC_FREQUENCIES,
                    timer_get_ticks_count() - req_start_time,
                    (uint32_t)status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_asic_frequencies_rsp_t))) {
        printf("dm_svc_perf_get_asic_frequencies: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       dm_svc_perf_get_dram_bw
*
*   DESCRIPTION
*
*       This function returns the DRAM BW
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
static void dm_svc_perf_get_dram_bw(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_dram_bw_rsp_t dm_rsp;
    struct dram_bw_t dram_bw;
    int status;

    status = get_module_dram_bw(&dram_bw);

    if(0 != status) {
        printf("perf mgmt error: get_module_dram_bw()\r\n");
    } else {
        dm_rsp.dram_bw = dram_bw;
    }

    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_DRAM_BANDWIDTH,
                    timer_get_ticks_count() - req_start_time,
                    (uint32_t)status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_dram_bw_rsp_t))) {
        printf("dm_svc_perf_get_dram_bw: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       dm_svc_perf_get_dram_capacity_util
*
*   DESCRIPTION
*
*       This function returns the % DRAM capacity utilization
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
static void dm_svc_perf_get_dram_capacity_util(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_dram_capacity_rsp_t dm_rsp;
    uint32_t pct_cap;
    int status;

    status = get_dram_capacity_percent(&pct_cap);

    if(0 != status) {
        printf("perf mgmt error: get_dram_capacity_percent()\r\n");
    } else {
        dm_rsp.percentage_cap.pct_cap = pct_cap;
    }

    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_DRAM_CAPACITY_UTILIZATION,
                    timer_get_ticks_count() - req_start_time,
                    (uint32_t)status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_dram_capacity_rsp_t))) {
        printf("dm_svc_perf_get_dram_capacity_util: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       dm_svc_perf_get_asic_per_core_util
*
*   DESCRIPTION
*
*       This function returns ASIC per core utilization
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
static void dm_svc_perf_get_asic_per_core_util(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_asic_per_core_util_rsp_t dm_rsp;
    uint8_t core_util=0;
    int status;
 
    status = get_asic_per_core_util(&core_util);
 
    if(0 != status) {
        printf("perf mgmt error: get_asic_per_core_util()\r\n");
    } else {
        dm_rsp.dummy = core_util;
    }
    
    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION,
                    timer_get_ticks_count() - req_start_time,
                    (uint32_t)status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_asic_per_core_util_rsp_t))) {
        printf("dm_svc_perf_get_asic_per_core_util: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       dm_svc_perf_get_asic_utilization
*
*   DESCRIPTION
*
*       This function returns the complete asic utilization
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
static void dm_svc_perf_get_asic_utilization(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_asic_per_core_util_rsp_t dm_rsp;
    uint8_t asic_util=0;
    int status;
 
    status = get_asic_utilization(&asic_util);
 
    if(0 != status) {
        printf("perf mgmt error: get_asic_utilization()\r\n");
    } else {
        dm_rsp.dummy = asic_util;
    }

    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_ASIC_UTILIZATION,
                    timer_get_ticks_count() - req_start_time,
                    (uint32_t)status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_asic_per_core_util_rsp_t))) {
        printf("dm_svc_perf_get_asic_utilization: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       dm_svc_perf_get_asic_stalls
*
*   DESCRIPTION
*
*       This function returns the total ASIC stalls
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
static void dm_svc_perf_get_asic_stalls(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_asic_stalls_rsp_t dm_rsp;
    uint8_t asic_stall=0;
    int status;
 
    status = get_asic_stalls(&asic_stall);
 
    if(0 != status) {
        printf("perf mgmt error: get_asic_stalls()\r\n");
    } else {
        dm_rsp.dummy = asic_stall;
    }
    
    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_ASIC_STALLS,
                    timer_get_ticks_count() - req_start_time,
                    (uint32_t)status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_asic_stalls_rsp_t))) {
        printf("dm_svc_perf_get_asic_stalls: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       dm_svc_perf_perf_get_asic_latency
*
*   DESCRIPTION
*
*       This function returns the ASIC latency
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
static void dm_svc_perf_get_asic_latency(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_asic_latency_rsp_t dm_rsp;
    uint8_t asic_latency=0;
    int status;
 
    status = get_asic_latency(&asic_latency);
 
    if(0 != status) {
        printf("perf mgmt error: get_asic_latency()\r\n");
    } else {
        dm_rsp.dummy = asic_latency;
    }

    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_ASIC_LATENCY,
                    timer_get_ticks_count() - req_start_time,
                    (uint32_t)status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_asic_latency_rsp_t))) {
        printf("dm_svc_perf_perf_get_asic_latency: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       process_performance_request
*
*   DESCRIPTION
*
*       This function handles the performance mgmt requests
*
*   INPUTS
*
*       msg_id    ID of serivces to handle
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void process_performance_request(tag_id_t tag_id, msg_id_t msg_id)
{
    uint64_t req_start_time = timer_get_ticks_count();

    switch (msg_id) {
    case DM_CMD_GET_ASIC_FREQUENCIES:
        dm_svc_perf_get_asic_frequencies(tag_id, req_start_time);
        break;
    case DM_CMD_GET_DRAM_BANDWIDTH:
        dm_svc_perf_get_dram_bw(tag_id, req_start_time);
        break;
    case DM_CMD_GET_DRAM_CAPACITY_UTILIZATION:
        dm_svc_perf_get_dram_capacity_util(tag_id, req_start_time);
        break;
    case DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION:
        dm_svc_perf_get_asic_per_core_util(tag_id, req_start_time);
        break;
    case DM_CMD_GET_ASIC_UTILIZATION:
        dm_svc_perf_get_asic_utilization(tag_id, req_start_time);
        break;
    case DM_CMD_GET_ASIC_STALLS:
        dm_svc_perf_get_asic_stalls(tag_id, req_start_time);
        break;
    case DM_CMD_GET_ASIC_LATENCY:
        dm_svc_perf_get_asic_latency(tag_id, req_start_time);
        break;
    default:
        printf("cmd_id: %d is not supported\r\n", msg_id);
        break;
    }
}

