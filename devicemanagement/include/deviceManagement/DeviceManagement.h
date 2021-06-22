//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_DEVICEMANAGEMENT_H
#define ET_DEVICEMANAGAMENT_H

/// @file

#include "deviceManagement/dm.h"
#include "device-layer/IDeviceLayer.h"

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <atomic>
#include <vector>
#include <unordered_map>

using namespace dev;

namespace device_management {

static std::unordered_map<std::string, device_mgmt_api::DM_CMD> const commandCodeTable = {
  {"DM_CMD_GET_MODULE_MANUFACTURE_NAME", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MANUFACTURE_NAME},
  {"DM_CMD_GET_MODULE_PART_NUMBER", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PART_NUMBER},
  {"DM_CMD_GET_MODULE_SERIAL_NUMBER", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_SERIAL_NUMBER},
  {"DM_CMD_GET_ASIC_CHIP_REVISION", device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_CHIP_REVISION},
  {"DM_CMD_GET_MODULE_PCIE_ADDR", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PCIE_ADDR},
  {"DM_CMD_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED},
  {"DM_CMD_GET_MODULE_MEMORY_SIZE_MB", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_SIZE_MB},
  {"DM_CMD_GET_MODULE_REVISION", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_REVISION},
  {"DM_CMD_GET_MODULE_FORM_FACTOR", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_FORM_FACTOR},
  {"DM_CMD_GET_MODULE_MEMORY_VENDOR_PART_NUMBER", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_VENDOR_PART_NUMBER},
  {"DM_CMD_GET_MODULE_MEMORY_TYPE", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MEMORY_TYPE},
  {"DM_CMD_GET_FUSED_PUBLIC_KEYS", device_mgmt_api::DM_CMD::DM_CMD_GET_FUSED_PUBLIC_KEYS},
  {"DM_CMD_GET_MODULE_FIRMWARE_REVISIONS", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_FIRMWARE_REVISIONS},
  {"DM_CMD_SET_FIRMWARE_UPDATE", device_mgmt_api::DM_CMD::DM_CMD_SET_FIRMWARE_UPDATE},
  {"DM_CMD_GET_FIRMWARE_BOOT_STATUS", device_mgmt_api::DM_CMD::DM_CMD_GET_FIRMWARE_BOOT_STATUS},
  {"DM_CMD_SET_SP_BOOT_ROOT_CERT", device_mgmt_api::DM_CMD::DM_CMD_SET_SP_BOOT_ROOT_CERT},
  {"DM_CMD_SET_SW_BOOT_ROOT_CERT", device_mgmt_api::DM_CMD::DM_CMD_SET_SW_BOOT_ROOT_CERT},
  {"DM_CMD_RESET_ETSOC", device_mgmt_api::DM_CMD::DM_CMD_RESET_ETSOC},
  {"DM_CMD_SET_FIRMWARE_VERSION_COUNTER", device_mgmt_api::DM_CMD::DM_CMD_SET_FIRMWARE_VERSION_COUNTER},
  {"DM_CMD_SET_FIRMWARE_VALID", device_mgmt_api::DM_CMD::DM_CMD_SET_FIRMWARE_VALID},
  {"DM_CMD_GET_MODULE_TEMPERATURE_THRESHOLDS", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_TEMPERATURE_THRESHOLDS},
  {"DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS", device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS},
  {"DM_CMD_GET_MODULE_POWER_STATE", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_POWER_STATE},
  {"DM_CMD_SET_MODULE_POWER_STATE", device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_POWER_STATE},
  {"DM_CMD_GET_MODULE_STATIC_TDP_LEVEL", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_STATIC_TDP_LEVEL},
  {"DM_CMD_SET_MODULE_STATIC_TDP_LEVEL", device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_STATIC_TDP_LEVEL},
  {"DM_CMD_GET_MODULE_CURRENT_TEMPERATURE", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_CURRENT_TEMPERATURE},
  {"DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES},
  {"DM_CMD_GET_MODULE_UPTIME", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_UPTIME},
  {"DM_CMD_GET_MODULE_VOLTAGE", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_VOLTAGE},
  {"DM_CMD_GET_MODULE_POWER", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_POWER},
  {"DM_CMD_GET_MODULE_MAX_TEMPERATURE", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_TEMPERATURE},
  {"DM_CMD_GET_MODULE_MAX_THROTTLE_TIME", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_THROTTLE_TIME},
  {"DM_CMD_GET_MODULE_MAX_DDR_BW", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_MAX_DDR_BW},
  {"DM_CMD_GET_MAX_MEMORY_ERROR", device_mgmt_api::DM_CMD::DM_CMD_GET_MAX_MEMORY_ERROR},
  {"DM_CMD_SET_DDR_ECC_COUNT", device_mgmt_api::DM_CMD::DM_CMD_SET_DDR_ECC_COUNT},
  {"DM_CMD_SET_PCIE_ECC_COUNT", device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_ECC_COUNT},
  {"DM_CMD_SET_SRAM_ECC_COUNT", device_mgmt_api::DM_CMD::DM_CMD_SET_SRAM_ECC_COUNT},
  {"DM_CMD_SET_PCIE_RESET", device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_RESET},
  {"DM_CMD_GET_MODULE_PCIE_ECC_UECC", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_PCIE_ECC_UECC},
  {"DM_CMD_GET_MODULE_DDR_BW_COUNTER", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_DDR_BW_COUNTER},
  {"DM_CMD_GET_MODULE_DDR_ECC_UECC", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_DDR_ECC_UECC},
  {"DM_CMD_GET_MODULE_SRAM_ECC_UECC", device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_SRAM_ECC_UECC},
  {"DM_CMD_SET_PCIE_MAX_LINK_SPEED", device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_MAX_LINK_SPEED},
  {"DM_CMD_SET_PCIE_LANE_WIDTH", device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_LANE_WIDTH},
  {"DM_CMD_SET_PCIE_RETRAIN_PHY", device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_RETRAIN_PHY},
  {"DM_CMD_GET_ASIC_FREQUENCIES", device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_FREQUENCIES},
  {"DM_CMD_GET_DRAM_BANDWIDTH", device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_BANDWIDTH},
  {"DM_CMD_GET_DRAM_CAPACITY_UTILIZATION", device_mgmt_api::DM_CMD::DM_CMD_GET_DRAM_CAPACITY_UTILIZATION},
  {"DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION", device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_PER_CORE_DATAPATH_UTILIZATION},
  {"DM_CMD_GET_ASIC_UTILIZATION", device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_UTILIZATION},
  {"DM_CMD_GET_ASIC_STALLS", device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_STALLS},
  {"DM_CMD_GET_ASIC_LATENCY", device_mgmt_api::DM_CMD::DM_CMD_GET_ASIC_LATENCY},
  {"DM_CMD_GET_MM_ERROR_COUNT", device_mgmt_api::DM_CMD::DM_CMD_GET_MM_ERROR_COUNT},
  {"DM_CMD_GET_DEVICE_ERROR_EVENTS", device_mgmt_api::DM_CMD::DM_CMD_GET_DEVICE_ERROR_EVENTS},
  {"DM_CMD_SET_DM_TRACE_RUN_CONTROL", device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_RUN_CONTROL},
  {"DM_CMD_SET_DM_TRACE_CONFIG", device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_CONFIG}};

static std::unordered_map<std::string, device_mgmt_api::POWER_STATE> const powerStateTable = {
  {"POWER_STATE_FULL", device_mgmt_api::POWER_STATE::POWER_STATE_FULL},
  {"POWER_STATE_REDUCED", device_mgmt_api::POWER_STATE::POWER_STATE_REDUCED},
  {"POWER_STATE_LOWEST", device_mgmt_api::POWER_STATE::POWER_STATE_LOWEST},
  {"POWER_STATE_INVALID", device_mgmt_api::POWER_STATE::POWER_STATE_INVALID}};

static std::unordered_map<std::string, device_mgmt_api::TDP_LEVEL> const TDPLevelTable = {
  {"TDP_LEVEL_ONE", device_mgmt_api::TDP_LEVEL::TDP_LEVEL_ONE},
  {"TDP_LEVEL_TWO", device_mgmt_api::TDP_LEVEL::TDP_LEVEL_TWO},
  {"TDP_LEVEL_THREE", device_mgmt_api::TDP_LEVEL::TDP_LEVEL_THREE},
  {"TDP_LEVEL_FOUR", device_mgmt_api::TDP_LEVEL::TDP_LEVEL_FOUR},
  {"TDP_LEVEL_INVALID", device_mgmt_api::TDP_LEVEL::TDP_LEVEL_INVALID}};

static std::unordered_map<std::string, device_mgmt_api::PCIE_RESET> const pcieResetTable = {
  {"PCIE_RESET_FLR", device_mgmt_api::PCIE_RESET::PCIE_RESET_FLR},
  {"PCIE_RESET_HOT", device_mgmt_api::PCIE_RESET::PCIE_RESET_HOT},
  {"PCIE_RESET_WARM", device_mgmt_api::PCIE_RESET::PCIE_RESET_WARM}};

static std::unordered_map<std::string, device_mgmt_api::PCIE_LINK_SPEED> const pcieLinkSpeedTable = {
  {"PCIE_LINK_SPEED_GEN3", device_mgmt_api::PCIE_LINK_SPEED::PCIE_LINK_SPEED_GEN3},
  {"PCIE_LINK_SPEED_GEN4", device_mgmt_api::PCIE_LINK_SPEED::PCIE_LINK_SPEED_GEN4}};

static std::unordered_map<std::string, device_mgmt_api::PCIE_LANE_W_SPLIT> const pcieLaneWidthTable = {
  {"PCIE_LANE_W_SPLIT_x4", device_mgmt_api::PCIE_LANE_W_SPLIT::PCIE_LANE_W_SPLIT_x4},
  {"PCIE_LANE_W_SPLIT_x8", device_mgmt_api::PCIE_LANE_W_SPLIT::PCIE_LANE_W_SPLIT_x8}};


struct lockable_;

struct dm_cmd {
  device_mgmt_api::dev_mgmt_cmd_header_t info;
  char payload[128];
};

struct dm_rsp {
  device_mgmt_api::dev_mgmt_rsp_header_t info;
  char payload[1];
};

typedef std::unordered_map<std::string, device_mgmt_api::DM_CMD>::const_iterator itCmd;

/// @class DeviceManagement
class DeviceManagement {
public:
  /// @brief Get instance to DeviceManagement
  ///
  /// @return DeviceManagement object
  static DeviceManagement &getInstance(IDeviceLayer *devLayer);
  
  /// @brief  Get total number of devices in system
  ///
  /// @return Total number of devices
  int getDevicesCount();
  
  /// @brief Send service request to device and wait for response
  ///
  /// @param[in] device_node  device index to use
  /// @param[in] cmd_code  Command code to service
  /// @param[inout] input_buff  pointer to data to send to device accompanying
  /// the command to service
  /// @param[in] input_size  size in bytes of the input_buff data
  /// @param[inout] output_buff  pointer to data received from the device after
  /// servicing the request
  /// @param[in] output_size  size in bytes of the output_buff data
  /// @param[inout] host_latency  Total time in miliseconds spent on the
  /// host side servicing a request; inclusive of dev_latency_micros
  /// @param[inout] dev_latency  Total time in microseconds spent on the
  /// device side servicing a request
  /// @param[in] timeout  Time to wait for the request to complete, i.e.
  /// to receive a message.
  ///
  /// @return Success of the request. Zero if the read was succesfull.
  int serviceRequest(const uint32_t device_node, uint32_t cmd_code, const char* input_buff, const uint32_t input_size,
                     char* output_buff, const uint32_t output_size, uint32_t* host_latency, uint64_t* dev_latency,
                     uint32_t timeout);

private:
  /// @brief DeviceManagement constructors
  DeviceManagement(){};
  // DeviceManagement(IDeviceLayer *devLayer){devLayer_ = devLayer;};
  DeviceManagement(const DeviceManagement &dm){};

  /// @brief DeviceManagement Destructor
  ~DeviceManagement(){};
  struct destruction_;

  /// @brief Determine if command is a 'set' or 'get' data command
  ///
  /// @param[in] cmd  Command code table iterator
  ///
  /// @return True if 'set' command
  bool isSetCommand(itCmd &cmd);

  /// @brief Determine if command code is a valid command
  ///
  /// @param[in] cmd_code  Command code to check
  ///
  /// @return Command code table iterator
  itCmd isValidCommand(uint32_t cmd_code);

  /// @brief Determine if device node is a valid node
  ///
  /// @param[in] device_node  Device node to check
  ///
  /// @return True if valid device node
  bool isValidDeviceNode(const uint32_t device_node);

  /// @brief Gets a mapped lockable device based on tokenized node data
  ///
  /// @param[in] index  index of device
  ///
  /// @return Smart shared pointer to lockable_ struct wrapping the device
  std::shared_ptr<lockable_> getDevice(const uint32_t index);

  /// @brief Fetch firmware image, verify, and write to FW region via MMIO
  ///
  /// @param[in] lockable  Smart shared pointer to lockable_ struct wrapping
  /// the device index
  /// @param[inout] filePath  Pointer to firmware image path on filesystem
  int processFirmwareImage(std::shared_ptr<lockable_> lockable, const char *filePath);

  /// @brief Determine if provided SHA512 is valid
  ///
  /// @param[in] str  Command code to check
  ///
  /// @return True if valid SHA512
  bool isValidSHA512(const std::string &str);

  /// @brief Read provided file and extract potential SHA512
  ///
  /// @param[inout] filePath  Pointer to hash file on filesystem
  ///
  /// @param[inout] hash  reference to unsigned char vector
  int processHashFile(const char *filePath, std::vector<unsigned char> &hash);

  /// @brief Get the provided IDeviceLayer pointer
  ///
  /// @return IDeviceLayer*
  IDeviceLayer* getDeviceLayer();

  /// @brief Set the provided IDeviceLayer pointer
  ///
  /// @param[in] ptr  IDeviceLayer pointer
  void setDeviceLayer(IDeviceLayer* ptr);

  std::unordered_map<uint32_t, std::shared_ptr<lockable_>> deviceMap_;
  IDeviceLayer *devLayer_;
  std::atomic<device_mgmt_api::tag_id_t> tag_id_;
};

typedef DeviceManagement &(*getDM_t)(IDeviceLayer *devLayer);

} // namespace device_management

#endif // ET_DEVICEMANAGEMENT_H
