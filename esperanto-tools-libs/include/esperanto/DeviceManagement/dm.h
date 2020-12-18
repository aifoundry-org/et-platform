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
  GET_MODULE_MANUFACTURE_NAME = 0x00,     // Asset Tracking
  GET_MODULE_PART_NUMBER,                 // Asset Tracking
  GET_MODULE_SERIAL_NUMBER,               // Asset Tracking
  GET_ASIC_CHIP_REVISION,                 // Asset Tracking
  GET_MODULE_DRIVER_REVISION,             // Asset Tracking
  GET_MODULE_PCIE_ADDR,                   // Asset Tracking
  GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED,    // Asset Tracking
  GET_MODULE_MEMORY_SIZE_MB,              // Asset Tracking
  GET_MODULE_REVISION,                    // Asset Tracking
  GET_MODULE_FORM_FACTOR,                 // Asset Tracking
  GET_MODULE_MEMORY_VENDOR_PART_NUMBER,   // Asset Tracking
  GET_MODULE_MEMORY_TYPE,                 // Asset Tracking
  GET_FUSED_PUBLIC_KEYS,                  // Asset Tracking
  GET_MODULE_FIRMWARE_REVISIONS,          // Firmware Update Service
  SET_FIRMWARE_UPDATE,                    // Firmware Update Service
  GET_FIRMWARE_BOOT_STATUS,               // Firmware Update Service
  SET_SP_BOOT_ROOT_CERT,                  // Firmware Update Service
  SET_SW_BOOT_ROOT_CERT,                  // Firmware Update Service
  RESET_ETSOC,                            // ET SOC Reset from PMIC
  SET_FIRMWARE_VERSION_COUNTER,           // Firmware Update Service
  SET_FIRMWARE_VALID,                     // Firmware Update Service
  GET_MODULE_TEMPERATURE_THRESHOLDS,      // Health Management (Thermal and Power)
  SET_MODULE_TEMPERATURE_THRESHOLDS,      // Health Management (Thermal and Power)
  GET_MODULE_POWER_STATE,                 // Health Management (Thermal and Power)
  SET_MODULE_POWER_STATE,                 // Health Management (Thermal and Power)
  GET_MODULE_STATIC_TDP_LEVEL,            // Health Management (Thermal and Power)
  SET_MODULE_STATIC_TDP_LEVEL,            // Health Management (Thermal and Power)
  GET_MODULE_CURRENT_TEMPERATURE,         // Health Management (Thermal and Power)
  GET_MODULE_TEMPERATURE_THROTTLE_STATUS, // Health Management (Thermal and Power)
  GET_MODULE_RESIDENCY_THROTTLE_STATES,   // Health Management (Thermal and Power)
  GET_MODULE_UPTIME,                      // Health Management (Thermal and Power)
  GET_MODULE_VOLTAGE,                     // Health Management (Thermal and Power)
  GET_MODULE_POWER,                       // Health Management (Thermal and Power)
  GET_MODULE_MAX_TEMPERATURE,             // Historical extreme values
  GET_MODULE_MAX_THROTTLE_TIME,           // Historical extreme values
  GET_MODULE_MAX_DDR_BW,                  // Historical extreme values
  GET_MAX_MEMORY_ERROR,                   // Historical extreme values
  SET_DDR_ECC_COUNT,                      // Error Control
  SET_PCIE_ECC_COUNT,                     // Error Control
  SET_SRAM_ECC_COUNT,                     // Error Control
  SET_PCIE_RESET,                         // Link Management
  GET_MODULE_PCIE_ECC_UECC,               // Link Management
  GET_MODULE_DDR_BW_COUNTER,              // Link Management
  GET_MODULE_DDR_ECC_UECC,                // Link Management
  GET_MODULE_SRAM_ECC_UECC,               // Link Management
  SET_PCIE_MAX_LINK_SPEED,                // Link Management
  SET_PCIE_LANE_WIDTH,                    // Link Management
  SET_PCIE_RETRAIN_PHY,                   // Link Management
  GET_ASIC_FREQUENCIES,                   // Performance
  GET_DRAM_BANDWIDTH,                     // Performance
  GET_DRAM_CAPACITY_UTILIZATION,          // Performance
  GET_ASIC_PER_CORE_DATAPATH_UTILIZATION, //  Performance
  GET_ASIC_UTILIZATION,                   // Performance
  GET_ASIC_STALLS,                        // Performance
  GET_ASIC_LATENCY,                       // Performance
  GET_MM_THREADS_STATE                    // Master Minion State
};

typedef uint8_t power_state_t;
enum power_state_e { FULL = 0x0, REDUCED, LOWEST, INVALID_POWER_STATE };

typedef uint8_t tdp_level_t;
enum tdp_level_e { LEVEL_1 = 0x0, LEVEL_2, LEVEL_3, LEVEL_4, INVALID_TDP_LEVEL };

struct temp_thresh_t {
  int32_t low_TempC;
  int32_t high_TempC;
};

typedef uint32_t pcie_reset_t;
enum pcie_reset_e { FLR = 0x0, HOT, WARM };

typedef uint32_t pcie_link_speed_t;
enum pcie_link_speed_e { GEN3 = 0x0, GEN4 };

typedef uint32_t pcie_lane_width_t;
enum pcie_lane_width_e { x4 = 0x0, x8 };

#define MAX_LENGTH 256

struct dmControlBlock {
  uint32_t cmd_id;
  uint64_t dev_latency;
  char cmd_payload[MAX_LENGTH];
}__packed__;

#endif // ET_RUNTIME_DM_H
