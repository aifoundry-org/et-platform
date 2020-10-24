#ifndef ASSET_TRACKING_SERVICE_H
#define ASSET_TRACKING_SERVICE_H

#include <stdint.h>
#include "mailbox.h"

#define MAX_LENGTH 8  

struct dm_control_block {
  uint32_t cmd_id;
  uint64_t dev_latency;
  char cmd_payload[8];
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

// PCIE gen bit rates(GT/s) definition
#define PCIE_GEN_1   2
#define PCIE_GEN_2   5
#define PCIE_GEN_3   8
#define PCIE_GEN_4   16
#define PCIE_GEN_5   32

// Function prototypes 
void asset_tracking_process_request(mbox_e mbox, uint32_t cmd_id);

#endif
