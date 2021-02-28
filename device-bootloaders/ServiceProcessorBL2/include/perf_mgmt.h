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

/*! \fn int get_dram_bw(struct dram_bw_t *dram_bw)
    \brief Interface to get the DRAM BW details
    \param struct dram_bw_t *dram_bw
    \returns int
*/
int get_dram_bw(struct dram_bw_t *dram_bw);

/*! \fn int get_max_dram_bw(struct max_dram_bw_t *max_dram_bw)
    \brief Interface to get the max DRAM BW details
    \param struct max_dram_bw_t *max_dram_bw
    \returns none
*/
int get_max_dram_bw(struct max_dram_bw_t *max_dram_bw);

/*! \fn struct max_dram_bw_t get_module_max_dram_bw_gbl(void)
    \brief Interface to get module's max DRAM BW details from global variable
    \param none
    \returns struct max_dram_bw_t
*/
struct max_dram_bw_t get_module_max_dram_bw_gbl(void);

/*! \fn struct dram_bw_t get_module_dram_bw_gbl(void)
    \brief Interface to get module's DRAM BW details from global variable
    \param none
    \returns struct dram_bw_t
*/
struct dram_bw_t get_module_dram_bw_gbl(void);

/*! \fn struct asic_frequencies_t get_module_asic_frequencies_gbl(void)
    \brief Interface to get module's ASIC frequencies from global variable
    \param none
    \returns struct dram_bw_t
*/
struct asic_frequencies_t get_module_asic_frequencies_gbl(void);

/*! \fn uint32_t get_dram_capacity_percent_gbl(void)
    \brief Interface to get module's DRAM Capacity percentage from global variable
    \param none
    \returns uint32_t
*/
uint32_t get_dram_capacity_percent_gbl(void);

/*! \fn uint8_t get_asic_per_core_util(void)
    \brief Interface to get ASIC's per core utilization details
    \param none
    \returns uint8_t
*/
uint8_t get_asic_per_core_util(void);

/*! \fn uint8_t get_asic_utilization(void)
    \brief Interface to get ASIC's complete utilization details
    \param none
    \returns uint8_t
*/
uint8_t get_asic_utilization(void);

/*! \fn uint8_t get_asic_stalls(void
    \brief Interface to get ASIC's stall details
    \param none
    \returns uint8_t
*/
uint8_t get_asic_stalls(void);

/*! \fn uint8_t get_asic_latency(void)
    \brief Interface to get ASIC's latency details
    \param none
    \returns uint8_t
*/
uint8_t get_asic_latency(void);