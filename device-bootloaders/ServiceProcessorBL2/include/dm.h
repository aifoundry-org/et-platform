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
#ifndef DEVICE_DM_H
#define DEVICE_DM_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bl2_timer.h"
#include "service_processor_BL2_data.h"

//TODO: Remove below define once HOST VQ support is completed. The dependency
//      of processing the command using mailbox should also be removed in asset_track.c and firmware_update.c.
#define MAILBOX_SUPPORTED

extern struct dm_control_block dm_cmd_rsp;

typedef uint32_t
    cmd_id_t; // Details of each Enum is defined here: https://gitlab.esperanto.ai/software/esperanto-tools-libs/blob/master/src/DeviceManagement/Schema/DeviceManagement-spec.schema.json#L2

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
    SET_SW_BOOT_ROOT_CERT,                // Firmware Update Service
    RESET_ETSOC,                          // ET SOC Reset from PMIC
    SET_FIRMWARE_VERSION_COUNTER,         // Firmware Update Service
    SET_FIRMWARE_VALID,                   // Firmware Update Service
    GET_MODULE_TEMPERATURE_THRESHOLDS,    // Health Management (Thermal and Power)
    SET_MODULE_TEMPERATURE_THRESHOLDS,    // Health Management (Thermal and Power)
    GET_MODULE_POWER_STATE,               // Health Management (Thermal and Power)
    SET_MODULE_POWER_STATE,               // Health Management (Thermal and Power)
    GET_MODULE_STATIC_TDP_LEVEL,          // Health Management (Thermal and Power)
    SET_MODULE_STATIC_TDP_LEVEL,          // Health Management (Thermal and Power)
    GET_MODULE_CURRENT_TEMPERATURE,       // Health Management (Thermal and Power)
    GET_MODULE_TEMPERATURE_THROTTLE_STATUS, // Health Management (Thermal and Power)
    GET_MODULE_RESIDENCY_THROTTLE_STATES, // Health Management (Thermal and Power)
    GET_MODULE_UPTIME,                    // Health Management (Thermal and Power)
    GET_MODULE_VOLTAGE,                   // Health Management (Thermal and Power)
    GET_MODULE_POWER,                     // Health Management (Thermal and Power)
    GET_MODULE_MAX_TEMPERATURE,           // Historical extreme values
    GET_MODULE_MAX_THROTTLE_TIME,         // Historical extreme values
    GET_MODULE_MAX_DDR_BW,                // Historical extreme values
    GET_MAX_MEMORY_ERROR,                 // Historical extreme values
    SET_DDR_ECC_COUNT,                    // Error Control
    SET_PCIE_ECC_COUNT,                   // Error Control
    SET_SRAM_ECC_COUNT,                   // Error Control
    SET_PCIE_RESET,                       // Link Management
    GET_MODULE_PCIE_ECC_UECC,             // Link Management
    GET_MODULE_DDR_BW_COUNTER,            // Link Management
    GET_MODULE_DDR_ECC_UECC,              // Link Management
    GET_MODULE_SRAM_ECC_UECC,             // Link Management
    SET_PCIE_MAX_LINK_SPEED,              // Link Management
    SET_PCIE_LANE_WIDTH,                  // Link Management
    SET_PCIE_RETRAIN_PHY,                 // Link Management
    GET_ASIC_FREQUENCIES,                 // Performance
    GET_DRAM_BANDWIDTH,                   // Performance
    GET_DRAM_CAPACITY_UTILIZATION,        // Performance
    GET_ASIC_PER_CORE_DATAPATH_UTILIZATION, //  Performance
    GET_ASIC_UTILIZATION,                 // Performance
    GET_ASIC_STALLS,                      // Performance
    GET_ASIC_LATENCY,                     // Performance
    GET_MM_THREADS_STATE                  // Master Minion State
};

typedef uint8_t power_state_t;
enum power_state_e { 
    FULL                = 0x0,
    REDUCED, 
    LOWEST,
    INVALID_POWER_STATE
};

typedef uint8_t tdp_level_t;
enum tdp_level_e { 
    LEVEL_1 = 0x0, 
    LEVEL_2, 
    LEVEL_3, 
    LEVEL_4, 
    INVALID_TDP_LEVEL
};

typedef uint8_t module_t;
enum module_e {
    MINION = 0x0,
    NOC,
    PSHIRE,
    IOSHIRE,
    MEMSHIRE,
    INVALID_MODULE
};

typedef uint32_t dm_status_t;
enum dm_status_e { 
    DM_STATUS_SUCCESS  = 0x0, 
    DM_STATUS_ERROR 
};

typedef uint32_t firmware_status_t;
enum firmware_status_e { 
    FIRMWARE_STATUS_PASS  = 0x0,
    FIRMWARE_STATUS_FAILED 
};

typedef uint32_t pcie_reset_t;
enum pcie_reset_e { 
      FLR = 0x0, 
      HOT, 
      WARM 
};

typedef uint32_t pcie_link_speed_t;
enum pcie_link_speed_e { 
      GEN3= 0x0, 
      GEN4 
};

typedef uint32_t pcie_lane_w_split_t;
enum pcie_lane_w_split_e { 
      x4= 0x0, 
      x8 
};

// Thresholds 
#define L0 0x0 // Low
#define HI 0x1 // High

#define MAX_LENGTH 256

struct dm_control_block {
    uint32_t cmd_id;
    uint64_t dev_latency;
    char cmd_payload[MAX_LENGTH];
} __packed__;

/// TODO: The following will be available in the auto-generated device_mngt_api.h
typedef uint16_t tag_id_t;
typedef uint16_t msg_id_t;

/// @brief Common header, common to command, response, and events
struct cmn_header_t {
    uint16_t size; ///< size of payload that follows the message header
    tag_id_t tag_id; ///< unique ID to correlate commands/responses across Host-> Device
    msg_id_t msg_id; ///< unique ID to differentiate commands/responses/events generated from host
};

/// @brief Command header for all commands host to device
struct cmd_header_t {
    struct cmn_header_t cmd_hdr; ///< Command header
    uint16_t flags; ///< flags bitmask, (1<<0) = barrier, (1<<1) = enable time stamps.
};

// DM request header. This header is attached to each request
struct cmd_hdr_t {
    cmd_id_t command_id; // Enum of the command ID as defined above
    uint32_t size; // Size of the Payload
};

// DM response header. This header is attached to each response
struct rsp_hdr_t {
    dm_status_t status;           // status of the Command execution
    uint64_t device_latency_usec; // Populated by the device during response
    uint32_t size;                // Size of the response Payload
};

struct asset_tracking_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct asset_tracking_t {
    char asset[8]; // Populated by the device during response
};

struct asset_tracking_rsp_t {
    struct rsp_hdr_t rsp_hdr;
    struct asset_tracking_t asset_info;
};

struct fused_pub_keys_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct fused_public_keys_t {
  uint8_t  keys[32];
  
};

struct fused_pub_keys_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    struct fused_public_keys_t  fused_public_keys; /// Fused public keys
};

struct firmware_versions_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct firmware_versions_t {
    uint32_t bl1_v;        // BL1 Firmware version
    uint32_t bl2_v;        // BL2 Firmware version
    uint32_t mm_v;         // Machine Minion Firmware version
    uint32_t wm_v;         // Worker Minion Firmware version
    uint32_t machm_v;      // Machine Minion Firmware version
};

struct firmware_versions_rsp_t {
    struct rsp_hdr_t rsp_hdr;
    struct firmware_versions_t firmware_versions;   
};

struct fw_image_path_t {
  char  path[64]; // Firmware image path
  
};
struct update_firmware_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
    struct fw_image_path_t  fw_image_path; /// Firmware image path.
};

struct certificate_hash_t {
  char  hash[32];
  
};
struct certificate_hash_cmd_t {
  struct cmd_header_t command_info;
  struct certificate_hash_t  certificate_hash; /// Certificate hash. Size needs to be actually 32 bytes.
};

struct temperature_threshold_t {
    uint8_t lo_temperature_c; // Low temperature threshold
    uint8_t hi_temperature_c; // High temperature threshold
};

struct temperature_threshold_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
    struct temperature_threshold_t temperature_threshold; 
};

struct temperature_threshold_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    struct temperature_threshold_t temperature_threshold; 
};

struct power_state_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
    power_state_t pwr_state; // Power State
};

struct power_state_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    power_state_t pwr_state; // Power State
};

struct tdp_level_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
    tdp_level_t tdp_level; // Static TDP Level
};

struct tdp_level_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    tdp_level_t tdp_level; // Static TDP Level
};

struct current_temperature_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct current_temperature_t {
  uint8_t  temperature_c;
  
};
struct current_temperature_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    uint8_t temperature_c; // Current temperature (in C)
};

struct throttle_time_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct throttle_time_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    uint64_t time_usec; // Throttle time in usec
};

struct module_power_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct module_power_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    uint8_t watts; // Module power in watts
};

struct module_voltage_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct module_voltage_t {
    uint16_t minion_shire_mV;
    uint16_t noc_mV;
    uint16_t mem_shire_mV;
    uint16_t ddr_mV;
    uint16_t pcie_shire_mV;
    uint16_t io_shire_mV;
}; 

struct module_voltage_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    struct module_voltage_t module_voltage; // Module voltage in volts
};

struct module_uptime_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct module_uptime_t {
    uint16_t day; // time day
    uint8_t  hours; // time hour
    uint8_t  mins; // time minutes
};

struct module_uptime_rsp_t {
    struct rsp_hdr_t rsp_hdr;
    struct module_uptime_t module_uptime;
};

struct max_temperature_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct max_temperature_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    uint8_t max_temperature_c; // Max Temperature(in c)
};

struct max_memory_error_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct max_ecc_count_t {
    uint8_t count; // Max ECC Count
};

struct max_memory_error_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    struct max_ecc_count_t max_ecc_count; 
};

struct max_dram_bw_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct max_dram_bw_t {
    uint8_t max_bw_rd_req_sec; // Max BW Read Req Secs
    uint8_t max_bw_wr_req_sec; // Max BW Write Req Secs
  
};

struct max_dram_bw_rsp_t {
    struct rsp_hdr_t rsp_hdr;
    struct max_dram_bw_t max_dram_bw;

};

struct max_throttle_time_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct max_throttle_time_t {
   uint64_t usec; // Max throttle time in usecs
};

struct max_throttle_time_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    struct max_throttle_time_t max_throttle_time;
};

struct set_error_count_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
    uint8_t ecc_error_count; // ECC Error count
};

struct pcie_reset_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
    pcie_reset_t reset_type; // PCIE reset type
};

struct get_error_count_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct errors_count_t {
    uint32_t ecc; // ECC Count
    uint32_t uecc; // UECC Count
};

struct get_error_count_rsp_t {
    struct rsp_hdr_t rsp_hdr;
    struct errors_count_t  errors_count;  

};

struct dram_bw_counter_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct dram_bw_counter_t {
    uint32_t read_req_per_sec; // Read Requests per second
    uint32_t write_req_per_sec; // Write Requests per second
};

struct dram_bw_counter_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    struct dram_bw_counter_t dram_bw_counter;
};

struct pcie_link_speed_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
    pcie_link_speed_t speed; // PCIE Link Speed
};

struct pcie_lane_width_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
    pcie_lane_w_split_t width; // PCIE Lane width
};

struct pcie_retrain_phy_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct etsoc_reset_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct asic_frequencies_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct asic_frequencies_t {
    uint32_t minion_shire_mhz;
    uint32_t noc_mhz;
    uint32_t mem_shire_mhz;
    uint32_t ddr_mhz;
    uint32_t pcie_shire_mhz;
    uint32_t io_shire_mhz;
};

struct asic_frequencies_rsp_t {
    struct rsp_hdr_t rsp_hdr;
    struct asic_frequencies_t asic_frequency; 
};

struct dram_bw_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct dram_bw_t {
    uint32_t read_req_sec;
    uint32_t write_req_sec;
};

struct dram_bw_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    struct dram_bw_t dram_bw;
};

struct dram_capacity_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct dram_capacity_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    uint32_t percentage_cap;
};

struct asic_per_core_util_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct asic_per_core_util_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    //TODO : Response fields to TBD
};

struct asic_stalls_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct asic_stalls_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    //TODO : Response fields to TBD
};

struct asic_latency_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct asic_latency_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    //TODO : Response fields to TBD
};

struct mm_state_cmd_t {
    struct cmd_hdr_t cmd_hdr; // Command header
};

struct mm_state_t {
    uint8_t master_thread_state;
    uint8_t vq_s_thread_state;
    uint8_t vq_c_thread_state;
    uint8_t vq_w_thread_state;
};

struct mm_state_rsp_t {
    struct rsp_hdr_t rsp_hdr; 
    struct mm_state_t mm_state;
};

#endif
