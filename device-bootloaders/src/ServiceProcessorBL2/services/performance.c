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
#include "mm_iface.h"
#include "trace.h"
#include "thermal_pwr_mgmt.h"

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
*       tag               tag id
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
    int32_t status;

    Log_Write(LOG_LEVEL_INFO, "Performance request: %s\n", __func__);

    status = get_module_asic_frequencies(&asic_frequencies);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, "perf mgmt error: get_module_asic_frequencies()\r\n");
    }
    else
    {
        dm_rsp.asic_frequency = asic_frequencies;
    }

    Log_Write(LOG_LEVEL_INFO, "Performance response: %s\n", __func__);

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_ASIC_FREQUENCIES,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp,
                                       sizeof(struct device_mgmt_asic_frequencies_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "dm_svc_perf_get_asic_frequencies: Cqueue push error!\n");
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
*       tag               tag id
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
    int32_t status;

    Log_Write(LOG_LEVEL_INFO, "Performance request: %s\n", __func__);

    status = get_module_dram_bw(&dram_bw);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, "perf mgmt error: get_module_dram_bw()\r\n");
    }
    else
    {
        dm_rsp.dram_bw = dram_bw;
    }

    Log_Write(LOG_LEVEL_INFO, "Performance response: %s\n", __func__);

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_DRAM_BANDWIDTH,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_dram_bw_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "dm_svc_perf_get_dram_bw: Cqueue push error!\n");
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
*       tag               tag id
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
    int32_t status;

    Log_Write(LOG_LEVEL_INFO, "Performance request: %s\n", __func__);

    status = get_dram_capacity_percent(&pct_cap);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, "perf mgmt error: get_dram_capacity_percent()\r\n");
    }
    else
    {
        dm_rsp.percentage_cap.pct_cap = pct_cap;
    }

    Log_Write(LOG_LEVEL_INFO, "Performance response: %s\n", __func__);

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_DRAM_CAPACITY_UTILIZATION,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 !=
        SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_dram_capacity_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "dm_svc_perf_get_dram_capacity_util: Cqueue push error!\n");
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
*       tag               tag id
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
    uint8_t core_util = 0;
    int32_t status;

    Log_Write(LOG_LEVEL_INFO, "Performance request: %s\n", __func__);

    status = get_asic_per_core_util(&core_util);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, "perf mgmt error: get_asic_per_core_util()\r\n");
    }
    else
    {
        dm_rsp.dummy = core_util;
    }

    Log_Write(LOG_LEVEL_INFO, "Performance response: %s\n", __func__);

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp,
                                       sizeof(struct device_mgmt_asic_per_core_util_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "dm_svc_perf_get_asic_per_core_util: Cqueue push error!\n");
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
*       tag               tag id
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
    uint8_t asic_util = 0;
    int32_t status;

    Log_Write(LOG_LEVEL_INFO, "Performance request: %s\n", __func__);

    status = get_asic_utilization(&asic_util);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, "perf mgmt error: get_asic_utilization()\r\n");
    }
    else
    {
        dm_rsp.dummy = asic_util;
    }

    Log_Write(LOG_LEVEL_INFO, "Performance response: %s\n", __func__);

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_ASIC_UTILIZATION,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp,
                                       sizeof(struct device_mgmt_asic_per_core_util_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "dm_svc_perf_get_asic_utilization: Cqueue push error!\n");
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
*       tag               tag id
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
    uint8_t asic_stall = 0;
    int32_t status;

    Log_Write(LOG_LEVEL_INFO, "Performance request: %s\n", __func__);

    status = get_asic_stalls(&asic_stall);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, "perf mgmt error: get_asic_stalls()\r\n");
    }
    else
    {
        dm_rsp.dummy = asic_stall;
    }

    Log_Write(LOG_LEVEL_INFO, "Performance response: %s\n", __func__);

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_ASIC_STALLS, timer_get_ticks_count() - req_start_time,
                    status);

    if (0 !=
        SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_asic_stalls_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "dm_svc_perf_get_asic_stalls: Cqueue push error!\n");
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
*       tag               tag id
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
    uint8_t asic_latency = 0;
    int32_t status;

    Log_Write(LOG_LEVEL_INFO, "Performance request: %s\n", __func__);

    status = get_asic_latency(&asic_latency);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, "perf mgmt error: get_asic_latency()\r\n");
    }
    else
    {
        dm_rsp.dummy = asic_latency;
    }

    Log_Write(LOG_LEVEL_INFO, "Performance response: %s\n", __func__);

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_ASIC_LATENCY, timer_get_ticks_count() - req_start_time,
                    status);

    if (0 !=
        SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_asic_latency_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "dm_svc_perf_perf_get_asic_latency: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       dm_svc_perf_get_mm_stats
*
*   DESCRIPTION
*
*       This function returns the current mm stats
*
*   INPUTS
*
*       tag               tag id
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void dm_svc_perf_get_mm_stats(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_get_mm_stats_rsp_t dm_rsp;
    int32_t status;

    Log_Write(LOG_LEVEL_INFO, "Performance request: %s\n", __func__);

    status = get_mm_stats(&dm_rsp.mm_stats);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, "perf mgmt error: dm_svc_perf_get_mm_stats()\r\n");
    }

    Log_Write(LOG_LEVEL_INFO, "Performance response: %s\n", __func__);

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_MM_STATS, timer_get_ticks_count() - req_start_time,
                    status)

    if (0 !=
        SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_get_mm_stats_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "dm_svc_perf_get_mm_stats: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*      dm_svc_perf_stats_run_control
*
*   DESCRIPTION
*
*       This function controls the enable, disable or reset of DM stats
*       (MM or SP) or reset of DM stats trace buffer
*
*   INPUTS
*
*       tag_id            Tag id
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*       buffer            Command input buffer
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void dm_svc_perf_stats_run_control(tag_id_t tag_id, uint64_t req_start_time, void *buffer)
{
    const struct device_mgmt_stats_run_control_cmd_t *dm_cmd =
        (struct device_mgmt_stats_run_control_cmd_t *)buffer;
    struct device_mgmt_default_rsp_t dm_rsp;
    sp2mm_stats_control_e control = 0;
    int32_t status = STATUS_SUCCESS;

    if (dm_cmd->type & STATS_TYPE_SP)
    {
        /* Check flag to Enable/Disable SP Trace logging. */
        if (dm_cmd->control & STATS_CONTROL_TRACE_ENABLE)
        {
            Trace_Run_Control_SP_Dev_Stats(TRACE_ENABLE);
        }
        else
        {
            Trace_Run_Control_SP_Dev_Stats(TRACE_DISABLE);
        }

        /* Check flag to reset SP stats trace buffer. */
        if (dm_cmd->control & STATS_CONTROL_RESET_TRACEBUF)
        {
            Trace_Reset_SP_Dev_Stats_Buffer();
        }

        /* Check flag to reset SP stats. */
        if (dm_cmd->control & STATS_CONTROL_RESET_COUNTER)
        {
            Thermal_Pwr_Mgmt_Init_OP_Stats();
        }
    }

    if (dm_cmd->type & STATS_TYPE_MM)
    {
        /* Check flag to Enable/Disable MM Trace logging. */
        if (dm_cmd->control & STATS_CONTROL_TRACE_ENABLE)
        {
            control |= MM_STATS_CONTROL_TRACE_ENABLE;
        }

        /* Check flag to reset MM stats trace buffer. */
        if (dm_cmd->control & STATS_CONTROL_RESET_TRACEBUF)
        {
            control |= MM_STATS_CONTROL_RESET_TRACEBUF;
        }

        /* Check flag to reset MM stats. */
        if (dm_cmd->control & STATS_CONTROL_RESET_COUNTER)
        {
            control |= MM_STATS_CONTROL_RESET_COUNTER;
        }

        status = MM_Iface_MM_Stats_Run_Control(control);
    }

    if (0 == (dm_cmd->type & (STATS_TYPE_SP | STATS_TYPE_MM)))
    {
        status = ERROR_INVALID_ARGUMENT;
    }

    FILL_RSP_HEADER(dm_rsp, tag_id, DM_CMD_SET_STATS_RUN_CONTROL,
                    timer_get_ticks_count() - req_start_time, status)

    if (STATUS_SUCCESS != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(dm_rsp)))
    {
        Log_Write(LOG_LEVEL_ERROR, "%s: Cqueue push error!\n", __func__);
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
*       tag_id    ID of tag
*       msg_id    ID of services to handle
*       buffer    Pointer to command buffer
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void process_performance_request(tag_id_t tag_id, msg_id_t msg_id, void *buffer)
{
    uint64_t req_start_time = timer_get_ticks_count();

    switch (msg_id)
    {
        case DM_CMD_GET_MM_STATS:
            dm_svc_perf_get_mm_stats(tag_id, req_start_time);
            break;
        case DM_CMD_SET_STATS_RUN_CONTROL:
            dm_svc_perf_stats_run_control(tag_id, req_start_time, buffer);
            break;
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
            Log_Write(LOG_LEVEL_ERROR, "cmd_id: %d is not supported\r\n", msg_id);
            break;
    }
}
