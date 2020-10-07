/*------------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

#ifndef ET_DEVICE_MANAGEMENT_H
#define ET_DEVICE_MANAGEMENT_H



//TODO : This file needs to be removed once auto generation is implemented. 

#include <stdint.h>

struct dmControlBlock *create_dmctrlblk(struct dmControlBlock *dmcntlblk, uint32_t cmd_id, uint32_t latency, uint32_t payload_size, char payload[]);


#define MAX_LENGTH 8  

struct dmControlBlock {
  uint32_t cmd_id;
  uint32_t dev_latency;
  char cmd_payload[0];
}__packed__;


enum CommandCode {
  GET_MODULE_MANUFACTURE_NAME = 0x00,   // Asset Tracking
  GET_MODULE_PART_NUMBER,               // Asset Tracking
  GET_MODULE_SERIAL_NUMBER,             // Asset Tracking
  GET_MODULE_ASSET_TAG,                 // Asset Tracking
  GET_ASIC_CHIP_REVISION,               // Asset Tracking
  GET_MODULE_FIRMWARE_REVISIONS,        // Asset Tracking
  GET_MODULE_DRIVER_REVISION,           // Asset Tracking
  GET_MODULE_PCIE_ADDR,                 // Asset Tracking
  GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED,  // Asset Tracking
  GET_MODULE_MEMORY_SIZE_MB,            // Asset Tracking
  GET_MODULE_REVISION,                  // Asset Tracking
  GET_MODULE_FORM_FACTOR,               // Asset Tracking
  GET_MODULE_MEMORY_VENDOR_PART_NUMBER, // Asset Tracking
  GET_MODULE_MEMORY_TYPE,               // Asset Tracking
  GET_FUSED_PUBLIC_KEYS,                // Asset Tracking
  /*                Unsupported right now
    GET_MODULE_POWER_STATE,                 // Health & Perf
    GET_MODULE_ALERTS,                      // Health & Perf
    GET_MODULE_TEMPERATURE_THRESHOLDS,      // Health & Perf
    GET_MODULE_CURRENT_TEMPERATURE,         // Health & Perf
    GET_MODULE_TEMPERATURE_THROTTLE_STATUS, // Health & Perf
    GET_MODULE_RESIDENCY_THROTTLE_STATES,   // Health & Perf
    GET_MODULE_MAX_TEMPERATURE,             // Health & Perf
    GET_MODULE_MEMORY_UECC,                 // Health & Perf
    GET_MODULE_MEMORY_ECC,                  // Health & Perf
    GET_MODULE_PCIE_CE_UCE,                 // Health & Perf
    GET_FIRMWARE_BOOT_STATUS,               // Health & Perf
    GET_MODULE_UPTIME,                      // Health & Perf
    GET_MODULE_VOLTAGE,                     // Health & Perf
    GET_MODULE_POWER,                       // Health & Perf
    GET_ASIC_FREQUENCIES,                   // Health & Perf
    GET_DRAM_BANDWIDTH,                     // Health & Perf
    GET_DRAM_CAPACITY_UTILIZATION,          // Health & Perf
    GET_ASIC_PER_CORE_DATAPATH_UTILIZATION, // Health & Perf
    GET_ASIC_UTILIZATION,                   // Health & Perf
    GET_ASIC_STALLS,                        // Health & Perf
    GET_ASIC_LATENCY,                       // Health & Perf
  */
};

#endif // ET_DEVICE_MANAGEMENT_H
