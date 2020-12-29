
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
    uint32_t dram_capacity_percent;
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
};

/* Standard Response for all DM services */
#define  FILL_RSP_HEADER(rsp, sts, sz, latency) (rsp).rsp_hdr_ext.status = sts; \
                                                (rsp).rsp_hdr_ext.device_latency_usec = latency; \
                                                (rsp).rsp_hdr.size = sz; \
                                                     

#endif
