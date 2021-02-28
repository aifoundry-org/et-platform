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
        
*/
/***********************************************************************/
#include "perf_mgmt.h"

/************************************************************************
*
*   FUNCTION
*
*       get_dram_bw
*
*   DESCRIPTION
*
*       This function gets the DRAM Bandwidth (Read/Write request) details.
*       It is invoked by DM task.
*
*   INPUTS
*
*       dram_bw    Pointer to DRAM BW Struct
*
*   OUTPUTS
*
*       int        Return status
*
***********************************************************************/
int get_dram_bw(struct dram_bw_t *dram_bw) {

    // TODO : Populate the valid DRAM BW value.
    // https://esperantotech.atlassian.net/browse/SW-6560
    dram_bw->read_req_sec = 100;
    dram_bw->write_req_sec = 100;

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_max_dram_bw
*
*   DESCRIPTION
*
*       This function gets the Max DRAM Bandwidth (Read/Write request) details.
*       It is invoked by DM task. 
*
*   INPUTS
*
*       max_dram_bw    Pointer to Max DRAM BW Struct
*
*   OUTPUTS
*
*       int            Return status
*
***********************************************************************/
int get_max_dram_bw(struct max_dram_bw_t *max_dram_bw) {

    // TODO : Populate the valid MAX DRAM BW value.
    // https://esperantotech.atlassian.net/browse/SW-6560.
    max_dram_bw->max_bw_rd_req_sec = 100;
    max_dram_bw->max_bw_wr_req_sec = 100;

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_max_dram_bw_gbl
*
*   DESCRIPTION
*
*       This function gets the Max DRAM Bandwidth (Read/Write request) details
*       from global variable.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       max_dram_bw_t    Max DRAM BW Struct
*
***********************************************************************/
struct max_dram_bw_t get_module_max_dram_bw_gbl(void) {
    return get_soc_perf_reg()->max_dram_bw;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_dram_bw_gbl
*
*   DESCRIPTION
*
*       This function gets the DRAM Bandwidth (Read/Write request) details
*       from global variable.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       dram_bw_t    DRAM BW Struct
*
***********************************************************************/
struct dram_bw_t get_module_dram_bw_gbl(void) {
    return get_soc_perf_reg()->dram_bw;
}

/************************************************************************
*
*   FUNCTION
*
*       get_module_asic_frequencies_gbl
*
*   DESCRIPTION
*
*       This function gets the ASIC frequency details
*       from global variable.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       asic_frequencies_t    ASIC Frequencies Struct
*
***********************************************************************/
struct asic_frequencies_t get_module_asic_frequencies_gbl(void) {
    return get_soc_perf_reg()->asic_frequency;
}

/************************************************************************
*
*   FUNCTION
*
*       get_dram_capacity_percent_gbl
*
*   DESCRIPTION
*
*       This function gets the DRAM Capacity utilization 
*       from global variable.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       uint32_t    DRAM Capacity in percentage
*
***********************************************************************/
uint32_t get_dram_capacity_percent_gbl(void) {
    return get_soc_perf_reg()->dram_capacity_percent;
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
*       None
*
*   OUTPUTS
*
*       uint8_t    Utilization percentage
*
***********************************************************************/
uint8_t get_asic_per_core_util(void) {
    // TODO : Finalize the payload and implement the function get_asic_per_core_util()
    // to return payload.
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
*       None
*
*   OUTPUTS
*
*       uint8_t    Utilization percentage
*
***********************************************************************/
uint8_t get_asic_utilization(void) {
    // TODO : Finalize the payload and implement the function get_asic_utilization()
    // to return payload
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
*       None
*
*   OUTPUTS
*
*       uint8_t 
*
***********************************************************************/
uint8_t get_asic_stalls(void) {
    // TODO : Finalize the payload and implement the function get_asic_stalls()
    // to return payload
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
*       None
*
*   OUTPUTS
*
*       uint8_t 
*
***********************************************************************/
uint8_t get_asic_latency(void) {
    // TODO : Finalize the payload and implement the function get_asic_latency()
    // to return payload
    return 0;
}