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
    \brief A C module that implements the Power Management services

    Public interfaces:
        
*/
/***********************************************************************/
#include "perf_mgmt.h"

int get_dram_bw(struct dram_bw_t *dram_bw) {

    // TODO : Populate the valid DRAM BW value.
    // https://esperantotech.atlassian.net/browse/SW-6560
    dram_bw->read_req_sec = 100;
    dram_bw->write_req_sec = 100;

    return 0;
}

int get_max_dram_bw(struct max_dram_bw_t *max_dram_bw) {

    // TODO : Populate the valid MAX DRAM BW value.
    // https://esperantotech.atlassian.net/browse/SW-6560.
    max_dram_bw->max_bw_rd_req_sec = 100;
    max_dram_bw->max_bw_wr_req_sec = 100;

    return 0;
}

struct max_dram_bw_t get_module_max_dram_bw_gbl(void) {
    return get_soc_perf_reg()->max_dram_bw;
}


struct dram_bw_t get_module_dram_bw_gbl(void) {
    return get_soc_perf_reg()->dram_bw;
}
