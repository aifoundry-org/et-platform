/*------------------------------------------------------------------------------
 * Copyright (C) 2019, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

#ifndef ET_DEVICE_MGMT_API_RPC_TYPES_H
#define ET_DEVICE_MGMT_API_RPC_TYPES_H

#include "esperanto/device-apis/device_apis_message_types.h"
#include <stdint.h>

/* Device Mgmt API structs */
/* Structs that are being used in other tests */

/*! \struct asset_info_t
    \brief Asset Information
*/
struct asset_info_t {
  char  asset[24]; /**<  */

} __attribute__((packed));

/*! \struct fused_public_keys_t
    \brief Fused Public Keys
*/
struct fused_public_keys_t {
  uint8_t  keys[32]; /**<  */

} __attribute__((packed));

/*! \struct firmware_version_t
    \brief Firmware versions of different minions
*/
struct firmware_version_t {
  uint32_t  bl1_v; /**< BL1 Firmware version */
  uint32_t  bl2_v; /**< BL2 Firmware version */
  uint32_t  mm_v; /**< Machine Minion Firmware version */
  uint32_t  wm_v; /**< Worker Minion Firmware version */
  uint32_t  machm_v; /**< Machine Minion Firmware version */
  uint32_t  fw_release_rev; /**< Firmware release revision */
  uint32_t  pmic_v; /**< PMIC Firmware version */
  uint32_t  pad; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct fw_image_path_t
    \brief Firmware image path
*/
struct fw_image_path_t {
  char  path[64]; /**<  */

} __attribute__((packed));

/*! \struct certificate_hash_t
    \brief Fused Public Keys
*/
struct certificate_hash_t {
  char  key_blob[48]; /**<  */
  char  associated_data[48]; /**<  */

} __attribute__((packed));

/*! \struct temperature_threshold_t
    \brief
*/
struct temperature_threshold_t {
  uint8_t  sw_temperature_c; /**< Low temperature threshold */
  uint8_t  pad[7]; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct current_temperature_t
    \brief
*/
struct current_temperature_t {
  int16_t  ioshire_current; /**< IOSHIRE current temperature (in C) */
  int16_t  ioshire_low; /**< IOSHIRE lowest temperature (in C) */
  int16_t  ioshire_high; /**< IOSHIRE highest temperature (in C) */
  int16_t  minshire_avg; /**< Mionion Shire average temperature (in C) */
  int16_t  minshire_low; /**< Mionion Shire lowest temperature (in C) */
  int16_t  minshire_high; /**< Mionion Shire highest temperature (in C) */
  uint8_t  pmic_sys; /**< PMIC SYS temperature (in C) */
  uint8_t  pad[3]; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct residency_t
    \brief
*/
struct residency_t {
  uint64_t  cumulative; /**< Time(in usecs) */
  uint64_t  average; /**< Time(in usecs) */
  uint64_t  maximum; /**< Time(in usecs) */
  uint64_t  minimum; /**< Time(in usecs) */

} __attribute__((packed));

/*! \struct module_power_t
    \brief
*/
struct module_power_t {
  uint16_t  power; /**< binary encoded */
  uint8_t  pad[6]; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct module_uptime_t
    \brief
*/
struct module_uptime_t {
  uint16_t  day; /**< time day */
  uint8_t  hours; /**< time hour */
  uint8_t  mins; /**< time minutes */
  uint8_t  pad[4]; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct max_temperature_t
    \brief
*/
struct max_temperature_t {
  uint8_t  max_temperature_c; /**< Max temperature (in C) */
  uint8_t  pad[7]; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct max_ecc_count_t
    \brief
*/
struct max_ecc_count_t {
  uint32_t  count; /**< Max ECC count */
  uint32_t  pad; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct max_dram_bw_t
    \brief
*/
struct max_dram_bw_t {
  uint32_t  max_bw_rd_req_sec; /**< Max BW Read Req Secs */
  uint32_t  max_bw_wr_req_sec; /**< Max BW Write Req Secs */

} __attribute__((packed));

/*! \struct max_throttle_time_t
    \brief
*/
struct max_throttle_time_t {
  uint64_t  time_usec; /**< Time(in usecs) */

} __attribute__((packed));

/*! \struct ecc_error_count_t
    \brief
*/
struct ecc_error_count_t {
  uint8_t  count; /**< ECC Error count */
  uint8_t  pad[7]; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct errors_count_t
    \brief
*/
struct errors_count_t {
  uint32_t  ecc; /**< ECC Count */
  uint32_t  uecc; /**< UECC Count */

} __attribute__((packed));

/*! \struct dram_bw_counter_t
    \brief
*/
struct dram_bw_counter_t {
  uint32_t  bw_rd_req_sec; /**< BW Read Req Secs */
  uint32_t  bw_wr_req_sec; /**< BW Write Req Secs */

} __attribute__((packed));

/*! \struct asic_frequencies_t
    \brief
*/
struct asic_frequencies_t {
  uint32_t  minion_shire_mhz; /**< Minion Shire frequency in MHz */
  uint32_t  noc_mhz; /**< NOC frequency in MHz */
  uint32_t  mem_shire_mhz; /**< Mem Shire frequency in MHz */
  uint32_t  ddr_mhz; /**< DDR frequency in MHz */
  uint32_t  pcie_shire_mhz; /**< PCIe Shire frequency in MHz */
  uint32_t  io_shire_mhz; /**< IO Shire frequency in MHz */

} __attribute__((packed));

/*! \struct dram_bw_t
    \brief
*/
struct dram_bw_t {
  uint32_t  read_req_sec; /**< Read Req Secs */
  uint32_t  write_req_sec; /**< Write Req Secs */

} __attribute__((packed));

/*! \struct percentage_cap_t
    \brief
*/
struct percentage_cap_t {
  uint32_t  pct_cap; /**< PCT cap */
  uint32_t  pad; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct mm_error_count_t
    \brief
*/
struct mm_error_count_t {
  uint32_t  hang_count; /**< MM Hang count */
  uint32_t  exception_count; /**< MM Exception count */

} __attribute__((packed));

/*! \struct asic_voltage_t
    \brief
*/
struct asic_voltage_t {
  uint16_t  ddr; /**< DDR Voltage */
  uint16_t  l2_cache; /**< L2 Cache Voltage */
  uint16_t  maxion; /**< Maxion's Voltage */
  uint16_t  minion; /**< Minion's Voltage */
  uint16_t  pshire_0p75; /**< PShire 0p75 Voltage */
  uint16_t  noc; /**< NOC's Voltage */
  uint16_t  ioshire_0p75; /**< IOShire 0p75 Voltage */
  uint16_t  vddqlp; /**< Vddlp Voltage */
  uint16_t  vddq; /**< Vddq Voltage */
  uint8_t  pad[6]; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct module_voltage_t
    \brief
*/
struct module_voltage_t {
  uint8_t  ddr; /**< DDR Voltage */
  uint8_t  l2_cache; /**< L2 Cache Voltage */
  uint8_t  maxion; /**< Maxion's Voltage */
  uint8_t  minion; /**< Minion's Voltage */
  uint8_t  pcie; /**< PCIe's Voltage */
  uint8_t  noc; /**< NOC's Voltage */
  uint8_t  pcie_logic; /**< PCIE Logic Voltage */
  uint8_t  vddqlp; /**< Vddlp Voltage */
  uint8_t  vddq; /**< Vddq Voltage */
  uint8_t  pad[7]; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct mdi_hart_selection_t
    \brief
*/
struct mdi_hart_selection_t {
  uint64_t  shire_id; /**< Shire ID */
  uint64_t  thread_mask; /**< Thread Mask */
  uint64_t  flags; /**< Flags that indicate, select/unselect/etc function */

} __attribute__((packed));

/*! \struct mdi_hart_control_t
    \brief
*/
struct mdi_hart_control_t {
  uint64_t  hart_id; /**< Hart ID */
  uint64_t  flags; /**< Flags that indicate action to perform, reset/halt/resume/status/etc */

} __attribute__((packed));

/*! \struct mdi_bp_control_t
    \brief
*/
struct mdi_bp_control_t {
  uint64_t  hart_id; /**< Hart ID */
  uint64_t  bp_address; /**< Address for breakpoint */
  uint64_t  mode; /**< Minion mode */
  uint64_t  bp_event_wait_timeout; /**< Wait time(in ms) for BP to be hit */
  uint64_t  flags; /**< Flags that indicate BP action, set/unset/etc */

} __attribute__((packed));

/*! \struct mdi_ss_control_t
    \brief
*/
struct mdi_ss_control_t {
  uint64_t  shire_mask; /**< Shire Mask */
  uint64_t  thread_mask; /**< Thread Mask */
  uint64_t  flags; /**< Flags that indicate ss actions, TBD */

} __attribute__((packed));

/*! \struct mdi_gpr_read_t
    \brief
*/
struct mdi_gpr_read_t {
  uint64_t  hart_id; /**< HART ID */
  uint32_t  gpr_index; /**< GPR Index */
  uint8_t  pad[4]; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct mdi_dump_gpr_t
    \brief
*/
struct mdi_dump_gpr_t {
  uint64_t  gpr[32]; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct mdi_gpr_write_t
    \brief
*/
struct mdi_gpr_write_t {
  uint64_t  hart_id; /**< HART ID */
  uint64_t  data; /**< Data to be written to GPR */
  uint32_t  gpr_index; /**< GPR Index */
  uint8_t  pad[4]; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct mdi_csr_read_t
    \brief
*/
struct mdi_csr_read_t {
  uint64_t  hart_id; /**< HART ID */
  uint32_t  csr_name; /**< CSR Name */
  uint8_t  pad[4]; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct mdi_csr_write_t
    \brief
*/
struct mdi_csr_write_t {
  uint64_t  hart_id; /**< HART ID */
  uint64_t  data; /**< Data to be written to CSR */
  uint32_t  csr_name; /**< CSR Name */
  uint8_t  pad[4]; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct mdi_mem_read_t
    \brief
*/
struct mdi_mem_read_t {
  uint64_t  address; /**< Memory address to be read from */
  uint64_t  hart_id; /**< HART ID */
  uint32_t  size; /**< Size of memory region */
  uint8_t  access_type; /**< Memory access type */
  uint8_t  pad[3]; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct mdi_mem_write_t
    \brief
*/
struct mdi_mem_write_t {
  uint64_t  address; /**< Memory address to be written to */
  uint64_t  data; /**< Data to be written */
  uint32_t  size; /**< Size of memory region */
  uint8_t  pad[4]; /**< Padding for alignment */

} __attribute__((packed));

/*! \struct get_sp_stats_t
    \brief
*/
struct get_sp_stats_t {
  uint16_t system_power_avg;       /**< System power average */
  uint16_t system_power_min;       /**< System power minimum */
  uint16_t system_power_max;       /**< System power maximum */
  uint16_t system_temperature_avg; /**< System temperature average */
  uint16_t system_temperature_min; /**< System temperature minimum */
  uint16_t system_temperature_max; /**< System temperature maximum */
  uint16_t minion_power_avg;       /**< Minion power average */
  uint16_t minion_power_min;       /**< Minion power minimum */
  uint16_t minion_power_max;       /**< Minion power maximum */
  uint16_t minion_temperature_avg; /**< Minion temperature average */
  uint16_t minion_temperature_min; /**< Minion temperature minimum */
  uint16_t minion_temperature_max; /**< Minion temperature maximum */
  uint16_t minion_voltage_avg;     /**< Minion voltage average */
  uint16_t minion_voltage_min;     /**< Minion voltage minimum */
  uint16_t minion_voltage_max;     /**< Minion voltage maximum */
  uint16_t minion_freq_avg;        /**< Minion frequency average */
  uint16_t minion_freq_min;        /**< Minion frequency minimum */
  uint16_t minion_freq_max;        /**< Minion frequency maximum */
  uint16_t sram_power_avg;         /**< SRAM power average */
  uint16_t sram_power_min;         /**< SRAM power minimum */
  uint16_t sram_power_max;         /**< SRAM power maximum */
  uint16_t sram_temperature_avg;   /**< SRAM temperature average */
  uint16_t sram_temperature_min;   /**< SRAM temperature minimum */
  uint16_t sram_temperature_max;   /**< SRAM temperature maximum */
  uint16_t sram_voltage_avg;       /**< SRAM voltage average */
  uint16_t sram_voltage_min;       /**< SRAM voltage minimum */
  uint16_t sram_voltage_max;       /**< SRAM voltage maximum */
  uint16_t sram_freq_avg;          /**< SRAM frequency average */
  uint16_t sram_freq_min;          /**< SRAM frequency minimum */
  uint16_t sram_freq_max;          /**< SRAM frequency maximum */
  uint16_t noc_power_avg;          /**< NOC power average */
  uint16_t noc_power_min;          /**< NOC power minimum */
  uint16_t noc_power_max;          /**< NOC power maximum */
  uint16_t noc_temperature_avg;    /**< NOC temperature average */
  uint16_t noc_temperature_min;    /**< NOC temperature minimum */
  uint16_t noc_temperature_max;    /**< NOC temperature maximum */
  uint16_t noc_voltage_avg;        /**< NOC voltage average */
  uint16_t noc_voltage_min;        /**< NOC voltage minimum */
  uint16_t noc_voltage_max;        /**< NOC voltage maximum */
  uint16_t noc_freq_avg;           /**< NOC frequency average */
  uint16_t noc_freq_min;           /**< NOC frequency minimum */
  uint16_t noc_freq_max;           /**< NOC frequency maximum */

} __attribute__((packed));

/*! \struct get_mm_stats_t
    \brief
*/
struct get_mm_stats_t {
  uint64_t  cm_utilization_avg; /**< Compute minion utilization average */
  uint64_t  cm_utilization_min; /**< Compute minion utilization minimum */
  uint64_t  cm_utilization_max; /**< Compute minion utilization maximum */
  uint64_t  cm_bw_avg; /**< Compute minion bandwidth average */
  uint64_t  cm_bw_min; /**< Compute minion bandwidth minimum */
  uint64_t  cm_bw_max; /**< Compute minion bandwidth maximum */
  uint64_t  pcie_dma_read_bw_avg; /**< PCIE DMA read bandwidth average */
  uint64_t  pcie_dma_read_bw_min; /**< PCIE DMA read bandwidth minimum */
  uint64_t  pcie_dma_read_bw_max; /**< PCIE DMA read bandwidth maximum */
  uint64_t  pcie_dma_write_bw_avg; /**< PCIE DMA write bandwidth average */
  uint64_t  pcie_dma_write_bw_min; /**< PCIE DMA write bandwidth minimum */
  uint64_t  pcie_dma_write_bw_max; /**< PCIE DMA write bandwidth maximum */
  uint64_t  pcie_dma_read_utilization_avg; /**< PCIE DMA read utilization average */
  uint64_t  pcie_dma_read_utilization_min; /**< PCIE DMA read utilization minimum */
  uint64_t  pcie_dma_read_utilization_max; /**< PCIE DMA read utilization maximum */
  uint64_t  pcie_dma_write_utilization_avg; /**< PCIE DMA write utilization average */
  uint64_t  pcie_dma_write_utilization_min; /**< PCIE DMA write utilization minimum */
  uint64_t  pcie_dma_write_utilization_max; /**< PCIE DMA write utilization maximum */
  uint64_t  ddr_read_bw_avg; /**< DDR read bandwidth average */
  uint64_t  ddr_read_bw_min; /**< DDR read bandwidth minimum */
  uint64_t  ddr_read_bw_max; /**< DDR read bandwidth maximum */
  uint64_t  ddr_write_bw_avg; /**< DDR write bandwidth average */
  uint64_t  ddr_write_bw_min; /**< DDR write bandwidth minimum */
  uint64_t  ddr_write_bw_max; /**< DDR write bandwidth maximum */
  uint64_t  l2_l3_read_bw_avg; /**< L2/L3 read bandwidth average */
  uint64_t  l2_l3_read_bw_min; /**< L2/L3 read bandwidth minimum */
  uint64_t  l2_l3_read_bw_max; /**< L2/L3 read bandwidth maximum */
  uint64_t  l2_l3_write_bw_avg; /**< L2/L3 write bandwidth average */
  uint64_t  l2_l3_write_bw_min; /**< L2/L3 write bandwidth minimum */
  uint64_t  l2_l3_write_bw_max; /**< L2/L3 write bandwidth maximum */

} __attribute__((packed));

struct shire_cache_config_t {
  uint16_t scp_size;  /* L2 SCP size */
  uint16_t l2_size;   /* L2 size*/
  uint16_t l3_size;   /* L3 size */
} __attribute__((packed));

/* The real Device Mgmt API RPC messages that we exchange */

/*! \struct device_mgmt_echo_cmd_t
    \brief Execute the echo test cmd
*/
struct device_mgmt_echo_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_echo_rsp_t
    \brief Response to echo test
*/
struct device_mgmt_echo_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint64_t  dummy; /**< Dummy field */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_default_rsp_t
    \brief Default response to be used if no specific response is defined for a cmd
*/
struct device_mgmt_default_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  int32_t  payload; /**< Payload for default rsp. Will usually contain the status of command execution. */
  uint32_t  pad; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_asset_tracking_cmd_t
    \brief Command for asset info request
*/
struct device_mgmt_asset_tracking_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_asset_tracking_rsp_t
    \brief Response for the asset info command
*/
struct device_mgmt_asset_tracking_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct asset_info_t  asset_info; /**< Asset details */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_fused_pub_keys_cmd_t
    \brief Command to request the public keys
*/
struct device_mgmt_fused_pub_keys_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_fused_pub_keys_rsp_t
    \brief Response to fused public keys command
*/
struct device_mgmt_fused_pub_keys_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct fused_public_keys_t  fused_public_keys; /**< Fused public keys */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_firmware_versions_cmd_t
    \brief Command to request the firmware version
*/
struct device_mgmt_firmware_versions_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_firmware_versions_rsp_t
    \brief Response for firmware version command
*/
struct device_mgmt_firmware_versions_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct firmware_version_t  firmware_version; /**< FW Version struct */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_update_firmware_cmd_t
    \brief Command for updating the firmware
*/
struct device_mgmt_update_firmware_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  struct fw_image_path_t  fw_image_path; /**< Firmware image path. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_update_firmware_rsp_t
    \brief Response for firmware update command
*/
struct device_mgmt_update_firmware_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint64_t  dummy; /**< Dummy Field */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_certificate_hash_cmd_t
    \brief Command to update the certificates hash in OTP
*/
struct device_mgmt_certificate_hash_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  struct certificate_hash_t  certificate_hash; /**< Certificate hash. Size needs to be actually 32 bytes. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_certificate_hash_rsp_t
    \brief Response for certificate hash update command
*/
struct device_mgmt_certificate_hash_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_temperature_threshold_cmd_t
    \brief Command to update the temperature threshold
*/
struct device_mgmt_temperature_threshold_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  struct temperature_threshold_t  temperature_threshold; /**< Temperature threshold struct. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_temperature_threshold_rsp_t
    \brief Response for temperature threshold update command
*/
struct device_mgmt_temperature_threshold_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct temperature_threshold_t  temperature_threshold; /**< Temperature threshold struct */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_power_state_cmd_t
    \brief Command to request power state
*/
struct device_mgmt_power_state_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  power_state_e  pwr_state; /**< Power State */
  uint8_t  pad[7]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_power_state_rsp_t
    \brief Response for power state command
*/
struct device_mgmt_power_state_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  power_state_e  pwr_state; /**< Power State */
  uint8_t  pad[7]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_active_power_management_cmd_t
    \brief Command to set active power management on or off
*/
struct device_mgmt_active_power_management_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  active_power_management_e  pwr_management; /**< Power Management on or off */
  uint8_t  pad[7]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_active_power_management_rsp_t
    \brief Response for active power management command
*/
struct device_mgmt_active_power_management_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_tdp_level_cmd_t
    \brief Command to request TDL Level
*/
struct device_mgmt_tdp_level_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint8_t  tdp_level; /**< TDP Level */
  uint8_t  pad[7]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_tdp_level_rsp_t
    \brief Response for TDL Level command
*/
struct device_mgmt_tdp_level_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint8_t  tdp_level; /**< TDP Level */
  uint8_t  pad[7]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_current_temperature_cmd_t
    \brief Command to get the current temperature
*/
struct device_mgmt_current_temperature_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_current_temperature_rsp_t
    \brief Response for current temperature command
*/
struct device_mgmt_current_temperature_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct current_temperature_t  current_temperature; /**< Current temperature (in C). */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_throttle_residency_cmd_t
    \brief Command to get the power throttle residency
*/
struct device_mgmt_throttle_residency_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  power_throttle_state_e  pwr_throttle_state; /**< Power Throttle State */
  uint8_t  pad[7]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_throttle_residency_rsp_t
    \brief Response for throttle residency command
*/
struct device_mgmt_throttle_residency_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct residency_t  throttle_residency; /**< Time in microseconds. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_module_power_cmd_t
    \brief Command to get the module power info
*/
struct device_mgmt_module_power_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_module_power_rsp_t
    \brief Response for module power command
*/
struct device_mgmt_module_power_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct module_power_t  module_power; /**< Module power (binary encoded) */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_get_asic_voltage_cmd_t
    \brief Command to get the asic voltage info
*/
struct device_mgmt_get_asic_voltage_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_get_asic_voltage_rsp_t
    \brief Response for get asic voltage command
*/
struct device_mgmt_get_asic_voltage_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct asic_voltage_t  asic_voltage; /**< asic voltage (binary encoded) */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_get_module_voltage_cmd_t
    \brief Command to get the module voltage info
*/
struct device_mgmt_get_module_voltage_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_get_module_voltage_rsp_t
    \brief Response for get module voltage command
*/
struct device_mgmt_get_module_voltage_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct module_voltage_t  module_voltage; /**< Module voltage (binary encoded) */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_set_module_voltage_cmd_t
    \brief Command to set the module voltage
*/
struct device_mgmt_set_module_voltage_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  module_e  type; /**< Module type */
  uint8_t  value; /**< Module voltage value (binary encoded) */
  uint8_t  pad[6]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_set_module_voltage_rsp_t
    \brief Response for set module voltage command
*/
struct device_mgmt_set_module_voltage_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_set_module_part_number_cmd_t
    \brief Command to set module part number
*/
struct device_mgmt_set_module_part_number_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint32_t  part_number; /**< Part number. */
  uint32_t  pad; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_module_uptime_cmd_t
    \brief Command to get the module uptime info
*/
struct device_mgmt_module_uptime_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_module_uptime_rsp_t
    \brief Response for module uptime command
*/
struct device_mgmt_module_uptime_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct module_uptime_t  module_uptime; /**< Module uptime */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_max_temperature_cmd_t
    \brief Command to get the max temperature
*/
struct device_mgmt_max_temperature_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_max_temperature_rsp_t
    \brief Response for max temperature command
*/
struct device_mgmt_max_temperature_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct max_temperature_t  max_temperature; /**< Max temperature (in C). */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_max_memory_error_cmd_t
    \brief Command to get the max memory error count
*/
struct device_mgmt_max_memory_error_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_max_memory_error_rsp_t
    \brief Response for max memory error count command
*/
struct device_mgmt_max_memory_error_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct max_ecc_count_t  max_ecc_count; /**< Max ECC Count */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_max_dram_bw_cmd_t
    \brief Command to get Max DRAM BW info
*/
struct device_mgmt_max_dram_bw_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_max_dram_bw_rsp_t
    \brief Response for Max DRAM BW info command
*/
struct device_mgmt_max_dram_bw_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct max_dram_bw_t  max_dram_bw; /**< Max DRAM BW info */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_power_residency_cmd_t
    \brief Command to get the power residency
*/
struct device_mgmt_power_residency_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  power_state_e  pwr_state; /**< Power State */
  uint8_t  pad[7]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_power_residency_rsp_t
    \brief Response for power_residency command
*/
struct device_mgmt_power_residency_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct residency_t  power_residency; /**< Time in microseconds. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_set_error_count_cmd_t
    \brief Command to set the error count
*/
struct device_mgmt_set_error_count_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  struct ecc_error_count_t  ecc_error_count; /**< ECC Error count */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_pcie_reset_cmd_t
    \brief Command for PCIE Reset
*/
struct device_mgmt_pcie_reset_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  pcie_reset_e  reset_type; /**< Reset Type */
  uint32_t  pad; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_get_error_count_cmd_t
    \brief Command to get ECC and UECC count
*/
struct device_mgmt_get_error_count_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_get_error_count_rsp_t
    \brief Response for ECC and UECC count command
*/
struct device_mgmt_get_error_count_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct errors_count_t  errors_count; /**< ECC and UECC Error count */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_dram_bw_counter_cmd_t
    \brief Command to get DRAM BW info
*/
struct device_mgmt_dram_bw_counter_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_dram_bw_counter_rsp_t
    \brief Response for DRAM BW info command
*/
struct device_mgmt_dram_bw_counter_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct dram_bw_counter_t  dram_bw_counter; /**< DRAM BW info */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_pcie_link_speed_cmd_t
    \brief Command for setting PCIE Link Speed
*/
struct device_mgmt_pcie_link_speed_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  pcie_link_speed_e  speed; /**< PCIE Link Speed */
  uint32_t  pad; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_pcie_lane_width_cmd_t
    \brief Command for PCIE Lane width
*/
struct device_mgmt_pcie_lane_width_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  pcie_lane_w_split_e  lane_width; /**< PCIE Lane Width */
  uint32_t  pad; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_pcie_retrain_phy_cmd_t
    \brief Command for PCIE Phy Retraining
*/
struct device_mgmt_pcie_retrain_phy_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_etsoc_reset_cmd_t
    \brief Command for ETSOC Reset
*/
struct device_mgmt_etsoc_reset_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_asic_frequencies_cmd_t
    \brief Command to get frequencies of different components of ETSOC
*/
struct device_mgmt_asic_frequencies_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_asic_frequencies_rsp_t
    \brief Response for asic_frequencies_cmd command
*/
struct device_mgmt_asic_frequencies_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct asic_frequencies_t  asic_frequency; /**< Frequencies of different components */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_set_frequency_cmd_t
    \brief Command to request frequency change
*/
struct device_mgmt_set_frequency_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint16_t  pll_freq; /**< PLL frequency */
  pll_id_e  pll_id; /**< PLL id */
  use_step_e  use_step_clock; /**< Use step clock for Minions */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_set_frequency_rsp_t
    \brief Response for power state command
*/
struct device_mgmt_set_frequency_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint64_t  dummy; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_dram_bw_cmd_t
    \brief Command to get DRAM BW Info
*/
struct device_mgmt_dram_bw_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_dram_bw_rsp_t
    \brief Response for DRAM BW command
*/
struct device_mgmt_dram_bw_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct dram_bw_t  dram_bw; /**<  */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_dram_capacity_cmd_t
    \brief Command to get DRAM Capacity Info
*/
struct device_mgmt_dram_capacity_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_dram_capacity_rsp_t
    \brief Response for DRAM Capacity command
*/
struct device_mgmt_dram_capacity_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct percentage_cap_t  percentage_cap; /**<  */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_asic_per_core_util_cmd_t
    \brief Command to get ASIC utilization Info
*/
struct device_mgmt_asic_per_core_util_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_asic_per_core_util_rsp_t
    \brief Response for ASIC utilization command
*/
struct device_mgmt_asic_per_core_util_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_asic_stalls_cmd_t
    \brief Command to get ASIC stalls Info
*/
struct device_mgmt_asic_stalls_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_asic_stalls_rsp_t
    \brief Response for ASIC stalls command
*/
struct device_mgmt_asic_stalls_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_asic_latency_cmd_t
    \brief Command to get ASIC Latency Info
*/
struct device_mgmt_asic_latency_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_asic_latency_rsp_t
    \brief Response for ASIC Latency command
*/
struct device_mgmt_asic_latency_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_mm_state_cmd_t
    \brief Command to get MM state Info
*/
struct device_mgmt_mm_state_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_mm_state_rsp_t
    \brief Response for MM state command
*/
struct device_mgmt_mm_state_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct mm_error_count_t  mm_error_count; /**< MM error count details. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_get_sp_stats_cmd_t
    \brief Command for get sp stats request
*/
struct device_mgmt_get_sp_stats_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_get_sp_stats_rsp_t
    \brief Response for get sp stats command
*/
struct device_mgmt_get_sp_stats_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct get_sp_stats_t  sp_stats; /**< SP operating point stats. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_get_mm_stats_cmd_t
    \brief Command for get mm stats request
*/
struct device_mgmt_get_mm_stats_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_get_mm_stats_rsp_t
    \brief Response for get mm stats command
*/
struct device_mgmt_get_mm_stats_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct get_mm_stats_t  mm_stats; /**< MM get compute performance stats. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_stats_run_control_cmd_t
    \brief Command to enable, disable, reset stats counter OR reset stats trace buffer.
*/
struct device_mgmt_stats_run_control_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  stats_type_e  type; /**< This is a bit mask, each bit corresponds a stats type.
        Bit 0 - 1 - apply command to SP stats
        Bit 1 - 1 - apply command to MM stats */
  stats_control_e  control; /**< This is a bit mask, each bit corresponds control a Stats Trace run control.
        Bit 0 - 0 - Disable - Stop dumping to stats trace buffer
                1 - Enable - Start dumping to stats trace buffer (Default)
        Bit 1 - 0 - Do not reset stats counter
                1 - Reset stats counter
        Bit 2 - 0 - Do not reset stats trace buffer
                1 - Reset stats trace buffer */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_trace_config_cmd_t
    \brief Configure the trace configuration
*/
struct device_mgmt_trace_config_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint32_t  event_mask; /**< This is a bit mask, each bit corresponds to a specific Event to trace */
  uint32_t  filter_mask; /**< This is a bit mask representing a list of filters for a given event to trace */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_trace_config_rsp_t
    \brief Trace configure command reply
*/
struct device_mgmt_trace_config_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  dm_status_e  status; /**< DM_STATUS_SUCCESS or non-zero error code */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_trace_run_control_cmd_t
    \brief Configure options to start, stop, OR fetch trace logs.
*/
struct device_mgmt_trace_run_control_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  trace_control_e  control; /**< This is a bit mask, each bit corresponds control a Trace run control.
        Bit 0 - 0 - Disable - instructs service processor to stop tracing
                1 - Enable - instructs service processor to start tracing (Default)
        Bit 1 - 0 - Dump to Trace Buffer (Default)
                1 - Redirect to UART */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_trace_run_control_rsp_t
    \brief Trace run control command reply
*/
struct device_mgmt_trace_run_control_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  dm_status_e  status; /**< DM_STATUS_SUCCESS or non-zero error code */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_power_throttle_config_cmd_t
    \brief Configure options to configure power throttling states.
*/
struct device_mgmt_power_throttle_config_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  power_throttle_state_e  power_state; /**< This is throttle power state value, states can be
POWER_THROTTLE_STATE_POWER_UP,
POWER_THROTTLE_STATE_POWER_DOWN,
POWER_THROTTLE_STATE_POWER_SAFE */
  uint8_t  pad[7]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_power_throttle_config_rsp_t
    \brief power throttle command response
*/
struct device_mgmt_power_throttle_config_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  dm_status_e  status; /**< DM_STATUS_SUCCESS or non-zero error code */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_get_shire_cache_config_cmd_t
    \brief Command to get shire cache configuration
*/
struct device_mgmt_get_shire_cache_config_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_get_shire_cache_config_rsp_t
    \brief Response for get shire cache config command
*/
struct device_mgmt_get_shire_cache_config_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct shire_cache_config_t  sc_config; /**< Shire cache config */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_set_shire_cache_config_cmd_t
    \brief Command to set the shire cache config
*/
struct device_mgmt_set_shire_cache_config_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint16_t scp_size;
  uint16_t l2_size;
  uint16_t l3_size;
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_set_shire_cache_config_rsp_t
    \brief Response for set shire cache config
*/
struct device_mgmt_set_shire_cache_config_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint64_t  dummy; /**< Dummy field. */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_hart_selection_cmd_t
    \brief Command to select/unselect HART
*/
struct mdi_hart_selection_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  struct mdi_hart_selection_t  cmd_attr; /**< Command attributes for HART selection */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_hart_selection_rsp_t
    \brief HART selection response
*/
struct mdi_hart_selection_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint32_t  status; /**< Indications if HART selection operation suceeded */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_hart_control_cmd_t
    \brief Command to control HARTs
*/
struct mdi_hart_control_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  struct mdi_hart_control_t  cmd_attr; /**< Command attributes for HART control */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_hart_control_rsp_t
    \brief HART control response
*/
struct mdi_hart_control_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint32_t  status; /**< Indications if HART control operation suceeded */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_bp_control_cmd_t
    \brief Breakpoint control command
*/
struct mdi_bp_control_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  struct mdi_bp_control_t  cmd_attr; /**< Command attributes for BP control */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_bp_control_rsp_t
    \brief Breakpoint control response
*/
struct mdi_bp_control_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint32_t  status; /**< Indications if BP control operation suceeded */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_ss_control_cmd_t
    \brief Single Step control command
*/
struct mdi_ss_control_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  struct mdi_ss_control_t  cmd_attr; /**< Command attributes for Single Step control */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_ss_control_rsp_t
    \brief Single Step control response
*/
struct mdi_ss_control_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint32_t  status; /**< Indications if SS control operation suceeded */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_read_gpr_cmd_t
    \brief GPR Read command
*/
struct mdi_read_gpr_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  struct mdi_gpr_read_t  cmd_attr; /**< Command attributes(hart_id and gpr index) for reading GPR */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_dump_gpr_cmd_t
    \brief GPR Dump command
*/
struct mdi_dump_gpr_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  hart_id; /**< Hart ID for reading GPRs */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_dump_gpr_rsp_t
    \brief GPR Dump Response
*/
struct mdi_dump_gpr_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  struct mdi_dump_gpr_t  gprs; /**< GPRs */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_read_gpr_rsp_t
    \brief GPR Read response
*/
struct mdi_read_gpr_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint64_t  data; /**< GPR register data */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_write_gpr_cmd_t
    \brief GPR Write command
*/
struct mdi_write_gpr_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  struct mdi_gpr_write_t  cmd_attr; /**< Command attributes(hart_id,gpr index, data) for writing GPR */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_write_gpr_rsp_t
    \brief GPR Write Response
*/
struct mdi_write_gpr_rsp_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  uint64_t  dummy; /**< Dummy field */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_read_csr_cmd_t
    \brief CSR Read command
*/
struct mdi_read_csr_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  struct mdi_csr_read_t  cmd_attr; /**< Command attributes(hart_id and csr name) for reading CSR */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_read_csr_rsp_t
    \brief CSR Read response
*/
struct mdi_read_csr_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint64_t  data; /**< Control and Status Register(CSR) data */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_write_csr_cmd_t
    \brief CSR Write command
*/
struct mdi_write_csr_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  struct mdi_csr_write_t  cmd_attr; /**< Command attributes(hart_id,csr name, data) for writing to CSR */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_write_csr_rsp_t
    \brief CSR Write response
*/
struct mdi_write_csr_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint64_t  dummy; /**< Dummy field */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_mem_read_cmd_t
    \brief Memory Read command
*/
struct mdi_mem_read_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  struct mdi_mem_read_t  cmd_attr; /**< Command attributes(memory address, size) for reading memory */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_mem_read_rsp_t
    \brief Memory Read response
*/
struct mdi_mem_read_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint64_t  data; /**< Memory content value */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_mem_write_cmd_t
    \brief Memory Write command
*/
struct mdi_mem_write_cmd_t {
  dev_mgmt_cmd_header_t command_info; /**< Command header */
  struct mdi_mem_write_t  cmd_attr; /**< Command attributes(memory address, size, data) for writing to memory */
} __attribute__((packed, aligned(8)));

/*! \struct mdi_mem_write_rsp_t
    \brief Memory Write response
*/
struct mdi_mem_write_rsp_t {
  struct dev_mgmt_rsp_header_t rsp_hdr;
  uint32_t  status; /**< Indications if memory write suceeded */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));

/*! \struct device_mgmt_mdi_bp_event_t
    \brief Breakpoint notification reported by the device
*/
struct device_mgmt_mdi_bp_event_t {
  struct evt_header_t event_info;
  mdi_event_type_e  event_type; /**< Event Type */
  uint8_t  pad[4]; /**< Padding for alignment */
} __attribute__((packed, aligned(8)));


#endif /* ET_DEVICE_MGMT_API_RPC_TYPES_H */