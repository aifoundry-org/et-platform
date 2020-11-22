/**
 * Copyright (c) 2018-present, Esperanto Technologies Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ET_RUNTIME_DM_H
#define ET_RUNTIME_DM_H

#include <stdint.h>

enum CommandCode {
  GET_MODULE_MANUFACTURE_NAME = 0x00,   // Asset Tracking
  GET_MODULE_PART_NUMBER,               // Asset Tracking
  GET_MODULE_SERIAL_NUMBER,             // Asset Tracking
  GET_ASIC_CHIP_REVISION,               // Asset Tracking
  GET_MODULE_DRIVER_REVISION,           // Asset Tracking
  GET_MODULE_PCIE_ADDR,                 // Asset Tracking
  GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED,  // Asset Tracking
  GET_MODULE_MEMORY_SIZE_MB,            // Asset Tracking
  GET_MODULE_REVISION,                  // Asset Tracking
  GET_MODULE_FORM_FACTOR,               // Asset Tracking
  GET_MODULE_MEMORY_VENDOR_PART_NUMBER, // Asset Tracking
  GET_MODULE_MEMORY_TYPE,               // Asset Tracking
  GET_FUSED_PUBLIC_KEYS,                // Asset Tracking
  GET_MODULE_FIRMWARE_REVISIONS,        // Firmware Update Service
  SET_FIRMWARE_UPDATE,                  // Firmware Update Service
  GET_FIRMWARE_BOOT_STATUS,             // Firmware Update Service
  SET_SP_BOOT_ROOT_CERT,                // Firmware Update Service
  SET_SW_BOOT_ROOT_CERT                 // Firmware Update Service
  /*                Unsupported right now
    SET_FIRMWARE_VERSION_COUNTER,           // Firmware Update Service
    SET_FIRMWARE_VALID,                     // Firmware Update Service
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

#define MAX_LENGTH 256

struct dmControlBlock {
  uint32_t cmd_id;
  uint64_t dev_latency;
  char cmd_payload[MAX_LENGTH];
}__packed__;

#endif // ET_RUNTIME_DM_H
