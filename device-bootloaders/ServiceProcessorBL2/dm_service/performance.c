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
*       This file implements the following performance Management services.
*
*   FUNCTIONS
*
*       Host Requested Services functions:
*
*       - process_performance_request
*       - dm_svc_perf_get_asic_frequencies
*       - dm_svc_perf_get_dram_bw
*       - dm_svc_perf_get_dram_capacity_util
*       - dm_svc_perf_get_asic_per_core_util
*       - dm_svc_perf_get_asic_utilization
*       - dm_svc_perf_get_asic_stalls
*       - dm_svc_perf_perf_get_asic_latency
*
***********************************************************************/
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
static void dm_svc_perf_get_asic_frequencies(uint64_t req_start_time)
{
    struct device_mgmt_asic_frequencies_rsp_t dm_rsp;

    /*  Get the frequencies of various domains from the
        globals set by the dm sampling task */
    dm_rsp.asic_frequency  = get_soc_perf_reg()->asic_frequency;

    FILL_RSP_HEADER(dm_rsp.rsp_hdr, DM_STATUS_SUCCESS,
                    sizeof(dm_rsp) - sizeof(struct dev_mgmt_rsp_header_t),
                    timer_get_ticks_count() - req_start_time);
   if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_asic_frequencies_rsp_t))) {
        printf("dm_svc_perf_get_asic_frequencies: Cqueue push error !\n");
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
static void dm_svc_perf_get_dram_bw(uint64_t req_start_time)
{
    struct device_mgmt_dram_bw_rsp_t dm_rsp;
    /* Get the BW of DRAM from the globals set by the dm sampling task */
    dm_rsp.dram_bw  = get_soc_perf_reg()->dram_bw;

    FILL_RSP_HEADER(dm_rsp.rsp_hdr, DM_STATUS_SUCCESS,
                    sizeof(dm_rsp) - sizeof(struct dev_mgmt_rsp_header_t),
                    timer_get_ticks_count() - req_start_time);
   if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_dram_bw_rsp_t))) {
        printf("dm_svc_perf_get_dram_bw: Cqueue push error !\n");

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
static void dm_svc_perf_get_dram_capacity_util(uint64_t req_start_time)
{
    struct device_mgmt_dram_capacity_rsp_t dm_rsp;
    /* Get the capacity of DRAM from the globals set by the dm sampling task */

    dm_rsp.percentage_cap.pct_cap = get_soc_perf_reg()->dram_capacity_percent;
    FILL_RSP_HEADER(dm_rsp.rsp_hdr, DM_STATUS_SUCCESS,
                    sizeof(dm_rsp) - sizeof(struct dev_mgmt_rsp_header_t),
                    timer_get_ticks_count() - req_start_time);
    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_dram_capacity_rsp_t))) {
        printf("dm_svc_perf_get_dram_capacity_util: Cqueue push error !\n");

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
static void dm_svc_perf_get_asic_per_core_util(uint64_t req_start_time)
{
    struct device_mgmt_asic_per_core_util_rsp_t dm_rsp;
    // TODO: payload is TBD

    FILL_RSP_HEADER(dm_rsp.rsp_hdr, DM_STATUS_SUCCESS,
                    sizeof(dm_rsp) - sizeof(struct dev_mgmt_rsp_header_t),
                    timer_get_ticks_count() - req_start_time);
    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_asic_per_core_util_rsp_t))) {
        printf("dm_svc_perf_get_asic_per_core_util: Cqueue push error !\n");

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
static void dm_svc_perf_get_asic_utilization(uint64_t req_start_time)
{
    struct device_mgmt_asic_per_core_util_rsp_t dm_rsp;
    // TODO: payload is TBD
    FILL_RSP_HEADER(dm_rsp.rsp_hdr, DM_STATUS_SUCCESS,
                    sizeof(dm_rsp) - sizeof(struct dev_mgmt_rsp_header_t),
                    timer_get_ticks_count() - req_start_time);
    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_asic_per_core_util_rsp_t))) {
        printf("dm_svc_perf_get_asic_utilization: Cqueue push error !\n");

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
static void dm_svc_perf_get_asic_stalls(uint64_t req_start_time)
{
    struct device_mgmt_asic_stalls_rsp_t dm_rsp;
    // TODO: payload is TBD
    FILL_RSP_HEADER(dm_rsp.rsp_hdr, DM_STATUS_SUCCESS,
                    sizeof(dm_rsp) - sizeof(struct dev_mgmt_rsp_header_t),
                    timer_get_ticks_count() - req_start_time);
    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_asic_stalls_rsp_t))) {
        printf("dm_svc_perf_get_asic_stalls: Cqueue push error !\n");

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
static void dm_svc_perf_get_asic_latency(uint64_t req_start_time)
{
    struct device_mgmt_asic_latency_rsp_t dm_rsp;
    // TODO: payload is TBD
    FILL_RSP_HEADER(dm_rsp.rsp_hdr, DM_STATUS_SUCCESS,
                    sizeof(dm_rsp) - sizeof(struct dev_mgmt_rsp_header_t),
                    timer_get_ticks_count() - req_start_time);
    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_asic_latency_rsp_t))) {
        printf("dm_svc_perf_perf_get_asic_latency: Cqueue push error !\n");

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
*       cmd_id    ID of serivces to handle
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void process_performance_request(uint32_t cmd_id)
{
    uint64_t req_start_time;
    req_start_time = timer_get_ticks_count();
    switch (cmd_id) {
        case DM_CMD_GET_ASIC_FREQUENCIES: {
            dm_svc_perf_get_asic_frequencies(req_start_time);
        } break;
        case DM_CMD_GET_DRAM_BANDWIDTH: {
            dm_svc_perf_get_dram_bw(req_start_time);
        } break;
        case DM_CMD_GET_DRAM_CAPACITY_UTILIZATION: {
            dm_svc_perf_get_dram_capacity_util(req_start_time);
        } break;
        case DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION: {
             dm_svc_perf_get_asic_per_core_util(req_start_time);
        } break;
        case DM_CMD_GET_ASIC_UTILIZATION: {
            dm_svc_perf_get_asic_utilization(req_start_time);
        } break;
        case DM_CMD_GET_ASIC_STALLS: {
            dm_svc_perf_get_asic_stalls(req_start_time);
        } break;
        case DM_CMD_GET_ASIC_LATENCY: {
            dm_svc_perf_get_asic_latency(req_start_time);
        } break;
        default:
          printf("cmd_id : %d  is not supported \r\n", cmd_id);
    }
}
