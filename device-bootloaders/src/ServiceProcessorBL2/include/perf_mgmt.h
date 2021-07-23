#ifndef __PERF_MGMT_H__
#define __PERF_MGMT_H__
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

/*! \fn volatile struct soc_perf_reg_t *get_soc_perf_reg(void)
    \brief Interface to get the SOC perf register
    \param none
    \returns struct soc_perf_reg_t *
*/
volatile struct soc_perf_reg_t *get_soc_perf_reg(void);

/*! \fn int update_dram_bw()
    \brief Interface to update the DRAM BW details
    \param none
    \returns int
*/
int update_dram_bw(void);

/*! \fn struct dram_bw_t get_module_dram_bw(void)
    \brief Interface to get module's DRAM BW details from global variable
    \param struct dram_bw_t *dram_bw
    \returns int
*/
int get_module_dram_bw(struct dram_bw_t *dram_bw);

/*! \fn int get_module_max_dram_bw(struct max_dram_bw_t *max_dram_bw)
    \brief Interface to get module's max DRAM BW details from global variable
    \param struct max_dram_bw_t *max_dram_bw
    \returns int
*/
int get_module_max_dram_bw(struct max_dram_bw_t *max_dram_bw);

/*! \fn int32_t Get_Minion_Frequency(void)
    \brief Interface to get Minion frequency from global variable
    \param N/A
    \returns 
*/
int32_t Get_Minion_Frequency(void);

/*! \fn void Update_Minion_Frequency_Global_Reg(int new_freq)
    \brief Interface to update Minion frequency in global variable
    \param New Frequency in Mhz
    \returns int
*/
void Update_Minion_Frequency_Global_Reg(int32_t new_freq);

/*! \fn int get_module_asic_frequencies(struct asic_frequencies_t *asic_frequencies)
    \brief Interface to get module's ASIC frequencies from global variable
    \param struct asic_frequencies_t *asic_frequencies
    \returns int
*/
int get_module_asic_frequencies(struct asic_frequencies_t *asic_frequencies);

/*! \fn int update_dram_capacity_percent()
    \brief Interface to update DRAM capacity percentage(utilization)
    \param none
    \returns int
*/
int update_dram_capacity_percent(void);

/*! \fn int get_dram_capacity_percent(uint32_t *dram_capacity)
    \brief Interface to get module's DRAM Capacity percentage from global variable
    \param uint32_t *dram_capacity
    \returns int
*/
int get_dram_capacity_percent(uint32_t *dram_capacity);

/*! \fn int get_asic_per_core_util(uint8_t *core_util)
    \brief Interface to get ASIC's per core utilization details
    \param uint8_t *core_util
    \returns int
*/
int get_asic_per_core_util(uint8_t *core_util);

/*! \fn int get_asic_utilization(uint8_t *asic_util)
    \brief Interface to get ASIC's complete utilization details
    \param uint8_t *asic_util
    \returns int
*/
int get_asic_utilization(uint8_t *asic_util);

/*! \fn uint8_t get_asic_stalls(void
    \brief Interface to get ASIC's stall details
    \param uint8_t *asic_stalls
    \returns int
*/
int get_asic_stalls(uint8_t *asic_stalls);

/*! \fn uint8_t get_asic_latency(void)
    \brief Interface to get ASIC's latency details
    \param uint8_t *asic_latency
    \returns int
*/
int get_asic_latency(uint8_t *asic_latency);

/*! \fn int get_last_update_ts(uint64_t *last_ts)
    \brief Interface to get last update timestamp
    \param uint64_t *last_ts
    \returns int
*/
int get_last_update_ts(uint64_t *last_ts);

/*! \fn void dump_perf_globals(void)
    \brief This function prints the performance globals
    \param none
    \returns none
*/
void dump_perf_globals(void);
#endif
