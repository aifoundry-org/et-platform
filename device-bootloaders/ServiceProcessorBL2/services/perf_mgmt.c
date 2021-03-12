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
/*! \file perf_mgmt.c
    \brief A C module that provides abstraction to Performance Management
     services

    Public interfaces:
    update_dram_bw
    get_module_dram_bw
    get_module_max_dram_bw
    get_module_asic_frequencies
    update_dram_capacity_percent
    get_dram_capacity_percent
    get_asic_per_core_util
    get_asic_utilization
    get_asic_stalls
    get_asic_latency
        
*/
/***********************************************************************/
#include "perf_mgmt.h"

struct soc_perf_reg_t {
    struct asic_frequencies_t asic_frequency;
    struct dram_bw_t dram_bw;
    struct max_dram_bw_t max_dram_bw;
    uint32_t dram_capacity_percent;
    uint64_t last_ts_min;
};

struct soc_perf_reg_t g_soc_perf_reg __attribute__((section(".data")));

volatile struct soc_perf_reg_t *get_soc_perf_reg(void)
{
    return &g_soc_perf_reg;
}

/************************************************************************
*
*   FUNCTION
*
*       update_dram_bw
*
*   DESCRIPTION
*
*       This function gets the DRAM Bandwidth (Read/Write request) details.
*       It is invoked by DM task.
*
*   INPUTS
*
*       none
*
*   OUTPUTS
*
*       int        Return status
*
***********************************************************************/
int update_dram_bw(void)
{
    // TODO : Populate the valid DRAM BW value. Read it from HW
    // https://esperantotech.atlassian.net/browse/SW-6560

    // Update the global variable. DRAM BW
    get_soc_perf_reg()->dram_bw.read_req_sec = 100;
    get_soc_perf_reg()->dram_bw.write_req_sec = 100;

    // Update the max DRAM BW values if condition met
    if (get_soc_perf_reg()->max_dram_bw.max_bw_rd_req_sec <
        get_soc_perf_reg()->dram_bw.read_req_sec) {
        //TODO : Make the size of both members same.
        get_soc_perf_reg()->max_dram_bw.max_bw_rd_req_sec =
            (uint8_t)get_soc_perf_reg()->dram_bw.read_req_sec;
    }

    if (get_soc_perf_reg()->max_dram_bw.max_bw_wr_req_sec <
        get_soc_perf_reg()->dram_bw.write_req_sec) {
        get_soc_perf_reg()->max_dram_bw.max_bw_wr_req_sec =
            (uint8_t)get_soc_perf_reg()->dram_bw.write_req_sec;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_dram_bw
*
*   DESCRIPTION
*
*       This function gets the DRAM Bandwidth (Read/Write request) details
*       from global variable.
*
*   INPUTS
*
*       dram_bw_t    Pointer to DRAM BW Struct
*
*   OUTPUTS
*
*       int            Return status
*
***********************************************************************/
int get_module_dram_bw(struct dram_bw_t *dram_bw)
{
    *dram_bw = get_soc_perf_reg()->dram_bw;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_max_dram_bw
*
*   DESCRIPTION
*
*       This function gets the Max DRAM Bandwidth (Read/Write request) details
*       from global variable.
*
*   INPUTS
*
*       max_dram_bw    Pointer to Max DRAM BW struct
*
*   OUTPUTS
*
*       int            Return status
*
***********************************************************************/
int get_module_max_dram_bw(struct max_dram_bw_t *max_dram_bw)
{
    *max_dram_bw = get_soc_perf_reg()->max_dram_bw;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_asic_frequencies
*
*   DESCRIPTION
*
*       This function gets the ASIC frequency details
*       from global variable.
*
*   INPUTS
*
*       asic_frequencies_t    Pointer to ASIC Frequencies Struct
*
*   OUTPUTS
*
*      int                    Return status
*
***********************************************************************/
int get_module_asic_frequencies(struct asic_frequencies_t *asic_frequencies)
{
    *asic_frequencies = get_soc_perf_reg()->asic_frequency;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       update_dram_capacity_percent
*
*   DESCRIPTION
*
*       This function updates the DRAM Capacity utilization 
*
*   INPUTS
*
*       None
*
*   OUTPUTS
* 
*       int          Return status
*
***********************************************************************/
int update_dram_capacity_percent(void)
{
    // TODO : Compute DRAM capacity utilization. Update the global.
    // https://esperantotech.atlassian.net/browse/SW-6608
    get_soc_perf_reg()->dram_capacity_percent = 80;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_dram_capacity_percent
*
*   DESCRIPTION
*
*       This function gets the DRAM Capacity utilization from global 
*       variable.
*
*   INPUTS
*
*       uint32_t*     Pointer to DRAM Capacity(in percentage)
*
*   OUTPUTS
* 
*       int           Return status
*
***********************************************************************/
int get_dram_capacity_percent(uint32_t *pct_cap)
{
    *pct_cap = get_soc_perf_reg()->dram_capacity_percent;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_asic_per_core_util
*
*   DESCRIPTION
*
*       This function gets the ASIC per core utilization details
*
*   INPUTS
*
*       uint8_t*    Pointer to Core Utilization(in percentage)
*
*   OUTPUTS
*
*       int         Return status
*
***********************************************************************/
int get_asic_per_core_util(uint8_t *core_util)
{
    // TODO : Finalize the payload and implement the function get_asic_per_core_util()
    // to return payload.
    // https://esperantotech.atlassian.net/browse/SW-6608
    (void)core_util;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_asic_utilization
*
*   DESCRIPTION
*
*       This function gets the complete ASIC utilization details
*
*   INPUTS
*
*       uint8_t*    Pointer to ASIC utilization(in percentage)
*
*   OUTPUTS
*
*       int         Return status
*
***********************************************************************/
int get_asic_utilization(uint8_t *asic_util)
{
    // TODO : Finalize the payload and implement the function get_asic_utilization()
    // to return payload
    // https://esperantotech.atlassian.net/browse/SW-6608
    (void)asic_util;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_asic_stalls
*
*   DESCRIPTION
*
*       This function gets the ASIC stalls details
*
*   INPUTS
*
*       uint8_t   Pointer to asic_stalls
*
*   OUTPUTS
*
*        int       Return status 
*
***********************************************************************/
int get_asic_stalls(uint8_t *asic_stalls)
{
    // TODO : Finalize the payload and implement the function get_asic_stalls()
    // to return payload
    // https://esperantotech.atlassian.net/browse/SW-6608
    (void)asic_stalls;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_asic_latency
*
*   DESCRIPTION
*
*       This function gets the ASIC latency details
*
*   INPUTS
*
*       uint8_t    Pointer to asic latency
*
*   OUTPUTS
*
*       int         Return status 
*
***********************************************************************/
int get_asic_latency(uint8_t *asic_latency)
{
    // TODO : Finalize the payload and implement the function get_asic_latency()
    // to return payload
    // https://esperantotech.atlassian.net/browse/SW-6608
    (void)asic_latency;
    return 0;
}