//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICEMANAGEMENT_H
#define ET_RUNTIME_DEVICEMANAGAMENT_H

/// @file

#include "esperanto/DeviceManagement/dm.h"

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>

namespace et_runtime::device {
class PCIeDevice;
}

namespace device_management {

static std::unordered_map<std::string, CommandCode> const commandCodeTable = {
  {"GET_MODULE_MANUFACTURE_NAME", CommandCode::GET_MODULE_MANUFACTURE_NAME},
  {"GET_MODULE_PART_NUMBER", CommandCode::GET_MODULE_PART_NUMBER},
  {"GET_MODULE_SERIAL_NUMBER", CommandCode::GET_MODULE_SERIAL_NUMBER},
  {"GET_ASIC_CHIP_REVISION", CommandCode::GET_ASIC_CHIP_REVISION},
  {"GET_MODULE_DRIVER_REVISION", CommandCode::GET_MODULE_DRIVER_REVISION},
  {"GET_MODULE_PCIE_ADDR", CommandCode::GET_MODULE_PCIE_ADDR},
  {"GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED", CommandCode::GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED},
  {"GET_MODULE_MEMORY_SIZE_MB", CommandCode::GET_MODULE_MEMORY_SIZE_MB},
  {"GET_MODULE_REVISION", CommandCode::GET_MODULE_REVISION},
  {"GET_MODULE_FORM_FACTOR", CommandCode::GET_MODULE_FORM_FACTOR},
  {"GET_MODULE_MEMORY_VENDOR_PART_NUMBER", CommandCode::GET_MODULE_MEMORY_VENDOR_PART_NUMBER},
  {"GET_MODULE_MEMORY_TYPE", CommandCode::GET_MODULE_MEMORY_TYPE},
  {"GET_FUSED_PUBLIC_KEYS", CommandCode::GET_FUSED_PUBLIC_KEYS},
  {"GET_MODULE_FIRMWARE_REVISIONS", CommandCode::GET_MODULE_FIRMWARE_REVISIONS},
  {"SET_FIRMWARE_UPDATE", CommandCode::SET_FIRMWARE_UPDATE},
  {"GET_FIRMWARE_BOOT_STATUS", CommandCode::GET_FIRMWARE_BOOT_STATUS},
  {"SET_SP_BOOT_ROOT_CERT", CommandCode::SET_SP_BOOT_ROOT_CERT},
  {"SET_SW_BOOT_ROOT_CERT", CommandCode::SET_SW_BOOT_ROOT_CERT},
  {"RESET_ETSOC", CommandCode::RESET_ETSOC},
  {"SET_FIRMWARE_VERSION_COUNTER", CommandCode::SET_FIRMWARE_VERSION_COUNTER},
  {"SET_FIRMWARE_VALID", CommandCode::SET_FIRMWARE_VALID},
  {"GET_MODULE_TEMPERATURE_THRESHOLDS", CommandCode::GET_MODULE_TEMPERATURE_THRESHOLDS},
  {"SET_MODULE_TEMPERATURE_THRESHOLDS", CommandCode::SET_MODULE_TEMPERATURE_THRESHOLDS},
  {"GET_MODULE_POWER_STATE", CommandCode::GET_MODULE_POWER_STATE},
  {"SET_MODULE_POWER_STATE", CommandCode::SET_MODULE_POWER_STATE},
  {"GET_MODULE_STATIC_TDP_LEVEL", CommandCode::GET_MODULE_STATIC_TDP_LEVEL},
  {"SET_MODULE_STATIC_TDP_LEVEL", CommandCode::SET_MODULE_STATIC_TDP_LEVEL},
  {"GET_MODULE_CURRENT_TEMPERATURE", CommandCode::GET_MODULE_CURRENT_TEMPERATURE},
  {"GET_MODULE_TEMPERATURE_THROTTLE_STATUS", CommandCode::GET_MODULE_TEMPERATURE_THROTTLE_STATUS},
  {"GET_MODULE_RESIDENCY_THROTTLE_STATES", CommandCode::GET_MODULE_RESIDENCY_THROTTLE_STATES},
  {"GET_MODULE_UPTIME", CommandCode::GET_MODULE_UPTIME},
  {"GET_MODULE_VOLTAGE", CommandCode::GET_MODULE_VOLTAGE},
  {"GET_MODULE_POWER", CommandCode::GET_MODULE_POWER},
  {"GET_MODULE_MAX_TEMPERATURE", CommandCode::GET_MODULE_MAX_TEMPERATURE},
  {"GET_MODULE_MAX_THROTTLE_TIME", CommandCode::GET_MODULE_MAX_THROTTLE_TIME},
  {"GET_MODULE_MAX_DDR_BW", CommandCode::GET_MODULE_MAX_DDR_BW},
  {"GET_MAX_MEMORY_ERROR", CommandCode::GET_MAX_MEMORY_ERROR},
  {"SET_DDR_ECC_COUNT", CommandCode::SET_DDR_ECC_COUNT},
  {"SET_PCIE_ECC_COUNT", CommandCode::SET_PCIE_ECC_COUNT},
  {"SET_SRAM_ECC_COUNT", CommandCode::SET_SRAM_ECC_COUNT},
  {"SET_PCIE_RESET", CommandCode::SET_PCIE_RESET},
  {"GET_MODULE_PCIE_ECC_UECC", CommandCode::GET_MODULE_PCIE_ECC_UECC},
  {"GET_MODULE_DDR_BW_COUNTER", CommandCode::GET_MODULE_DDR_BW_COUNTER},
  {"GET_MODULE_DDR_ECC_UECC", CommandCode::GET_MODULE_DDR_ECC_UECC},
  {"GET_MODULE_SRAM_ECC_UECC", CommandCode::GET_MODULE_SRAM_ECC_UECC},
  {"SET_PCIE_MAX_LINK_SPEED", CommandCode::SET_PCIE_MAX_LINK_SPEED},
  {"SET_PCIE_LANE_WIDTH", CommandCode::SET_PCIE_LANE_WIDTH},
  {"SET_PCIE_RETRAIN_PHY", CommandCode::SET_PCIE_RETRAIN_PHY},
  {"GET_ASIC_FREQUENCIES", CommandCode::GET_ASIC_FREQUENCIES},
  {"GET_DRAM_BANDWIDTH", CommandCode::GET_DRAM_BANDWIDTH},
  {"GET_DRAM_CAPACITY_UTILIZATION", CommandCode::GET_DRAM_CAPACITY_UTILIZATION},
  {"GET_ASIC_PER_CORE_DATAPATH_UTILIZATION", CommandCode::GET_ASIC_PER_CORE_DATAPATH_UTILIZATION},
  {"GET_ASIC_UTILIZATION", CommandCode::GET_ASIC_UTILIZATION},
  {"GET_ASIC_STALLS", CommandCode::GET_ASIC_STALLS},
  {"GET_ASIC_LATENCY", CommandCode::GET_ASIC_LATENCY},
  {"GET_MM_THREADS_STATE", CommandCode::GET_MM_THREADS_STATE}};

static std::unordered_map<std::string, power_state_e> const powerStateTable = {
  {"FULL", power_state_e::FULL},
  {"REDUCED", power_state_e::REDUCED},
  {"LOWEST", power_state_e::LOWEST},
  {"INVALID_POWER_STATE", power_state_e::INVALID_POWER_STATE}};

static std::unordered_map<std::string, tdp_level_e> const TDPLevelTable = {
  {"LEVEL_1", tdp_level_e::LEVEL_1},
  {"LEVEL_2", tdp_level_e::LEVEL_2},
  {"LEVEL_3", tdp_level_e::LEVEL_3},
  {"LEVEL_4", tdp_level_e::LEVEL_4},
  {"INVALID_TDP_LEVEL", tdp_level_e::INVALID_TDP_LEVEL}};

static std::unordered_map<std::string, pcie_reset_e> const pcieResetTable = {
  {"FLR", pcie_reset_e::FLR}, {"HOT", pcie_reset_e::HOT}, {"WARM", pcie_reset_e::WARM}};

static std::unordered_map<std::string, pcie_link_speed_e> const pcieLinkSpeedTable = {
  {"GEN3", pcie_link_speed_e::GEN3}, {"GEN4", pcie_link_speed_e::GEN4}};

static std::unordered_map<std::string, pcie_lane_width_e> const pcieLaneWidthTable = {{"x4", pcie_lane_width_e::x4},
                                                                                      {"x8", pcie_lane_width_e::x8}};

struct lockable_;

typedef std::unordered_map<std::string, CommandCode>::const_iterator itCmd;

/// @class DeviceManagement
class DeviceManagement {
public:
  /// @brief Get instance to DeviceManagement
  ///
  /// @return DeviceManagement object
  static DeviceManagement &getInstance();

  /// @brief Send service request to device and wait for response
  ///
  /// @param[in] device_node  Char pointer to name of device node to open
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
  int serviceRequest(const char *device_node, uint32_t cmd_code,
                     const char *input_buff, const uint32_t input_size,
                     char *output_buff, const uint32_t output_size,
                     uint32_t *host_latency, uint64_t *dev_latency,
                     uint32_t timeout);

private:
  /// @brief DeviceManagement constructors
  DeviceManagement(){};
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
  bool isValidDeviceNode(const char *device_node);

  /// @brief Extract an index and whether it is a mgmt node
  ///
  /// @param[in] device_node  Char pointer to name of device node
  ///
  /// @return Tuple containing the index and whether node is a mgmt node
  std::tuple<uint32_t, bool> tokenizeDeviceNode(const char *device_node);

  /// @brief Gets a mapped lockable device based on tokenized node data
  ///
  /// @param[in] t  Tuple containing index and whether node is a mgmt node
  ///
  /// @return Smart shared pointer to lockable_ struct wrapping the device
  std::shared_ptr<lockable_> getDevice(const std::tuple<uint32_t, bool> &t);

  /// @brief Fetch firmware image, verify, and write to FW region via MMIO
  ///
  /// @param[in] lockable  Smart shared pointer to lockable_ struct wrapping
  /// the device
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

  std::unordered_map<uint32_t, std::shared_ptr<lockable_>[2]> deviceMap_;
};

typedef DeviceManagement &(*getDM_t)();

} // namespace device_management

#endif // ET_RUNTIME_DEVICEMANAGEMENT_H
