
/*-------------------------------------------------------------------------
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef __DM_CONFIG_H__
#define __DM_CONFIG_H__

#include <stdint.h>
#include <esperanto/device-apis/management-api/device_mgmt_api_rpc_types.h>

struct soc_perf_reg_t {
    struct asic_frequencies_t asic_frequency;
    struct dram_bw_t dram_bw;
    struct max_dram_bw_t  max_dram_bw;
    uint32_t dram_capacity_percent;
    uint64_t last_ts_min;
};

struct soc_power_reg_t {
     power_state_e module_power_state;
     tdp_level_e module_tdp_level;
     struct temperature_threshold_t temperature_threshold;
     uint8_t soc_temperature;
     uint8_t soc_power;
     uint8_t max_temp;
     struct module_uptime_t module_uptime;
     struct module_voltage_t module_voltage;
     uint64_t throttled_states_residency;
     uint64_t max_throttled_states_residency;
};

/* Response Header of different response structs in DM services */
#define  FILL_RSP_HEADER(rsp, tag, msg, latency, sts) (rsp).rsp_hdr.rsp_hdr.tag_id = tag; \
                                                      (rsp).rsp_hdr.rsp_hdr.msg_id = msg; \
                                                      (rsp).rsp_hdr.rsp_hdr.size = sizeof(rsp); \
                                                      (rsp).rsp_hdr.rsp_hdr_ext.device_latency_usec = latency; \
                                                      (rsp).rsp_hdr.rsp_hdr_ext.status = sts;
                                                     
                                                     

#endif
