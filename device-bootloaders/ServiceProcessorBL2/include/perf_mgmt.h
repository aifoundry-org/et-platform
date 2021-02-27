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
/*! \file perf_mgmt.h
    \brief A C header that defines the Performance manangement service's
    public interfaces. These interfaces provide services using which
    the host can query device for performance related details.
*/
/***********************************************************************/
#include "dm.h"
#include "dm_service.h"
#include "dm_task.h"

int get_dram_bw(struct dram_bw_t *dram_bw);
int get_max_dram_bw(struct max_dram_bw_t *max_dram_bw);
struct max_dram_bw_t get_module_max_dram_bw_gbl(void);
struct dram_bw_t get_module_dram_bw_gbl(void);