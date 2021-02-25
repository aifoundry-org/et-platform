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
/*! \file firmware_update.c
    \brief A C module that implements the performance Management services

    Public interfaces:
        process_performance_request
*/
/***********************************************************************/
#include <stdint.h>
#include "dm.h"
#include "sp_host_iface.h"
#include "bl2_perf.h"
#include "dm_task.h"

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

    /*  Get the frequencies of various domains from the
        globals set by the dm sampling task */
    dm_rsp.asic_frequency  = get_soc_perf_reg()->asic_frequency;

    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_ASIC_FREQUENCIES,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

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
    /* Get the BW of DRAM from the globals set by the dm sampling task */
    dm_rsp.dram_bw  = get_soc_perf_reg()->dram_bw;

    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_DRAM_BANDWIDTH,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

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
    /* Get the capacity of DRAM from the globals set by the dm sampling task */

    dm_rsp.percentage_cap.pct_cap = get_soc_perf_reg()->dram_capacity_percent;

    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_DRAM_CAPACITY_UTILIZATION,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

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
    // TODO: payload is TBD
    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    dm_rsp.dummy = DM_STATUS_SUCCESS;

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
    // TODO: payload is TBD
    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_ASIC_UTILIZATION,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    dm_rsp.dummy = DM_STATUS_SUCCESS;

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
    // TODO: payload is TBD
    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_ASIC_STALLS,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    dm_rsp.dummy = DM_STATUS_SUCCESS;

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
    // TODO: payload is TBD
    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_ASIC_LATENCY,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    dm_rsp.dummy = DM_STATUS_SUCCESS;

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
