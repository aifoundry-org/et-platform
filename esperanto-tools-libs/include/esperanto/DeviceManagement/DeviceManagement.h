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
#include <unordered_map>

namespace et_runtime::device {
class PCIeDevice;
}

namespace device_management {

static std::unordered_map<std::string, CommandCode> const commandCodeTable = {
  {"GET_MODULE_MANUFACTURE_NAME", CommandCode::GET_MODULE_MANUFACTURE_NAME},
  {"GET_MODULE_PART_NUMBER", CommandCode::GET_MODULE_PART_NUMBER},
  {"GET_MODULE_SERIAL_NUMBER", CommandCode::GET_MODULE_SERIAL_NUMBER},
  {"GET_MODULE_ASSET_TAG", CommandCode::GET_MODULE_ASSET_TAG},
  {"GET_ASIC_CHIP_REVISION", CommandCode::GET_ASIC_CHIP_REVISION},
  {"GET_MODULE_FIRMWARE_REVISIONS",
    CommandCode::GET_MODULE_FIRMWARE_REVISIONS},
  {"GET_MODULE_DRIVER_REVISION", CommandCode::GET_MODULE_DRIVER_REVISION},
  {"GET_MODULE_PCIE_ADDR", CommandCode::GET_MODULE_PCIE_ADDR},
  {"GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED",
    CommandCode::GET_MODULE_PCIE_NUM_PORTS_MAX_SPEED},
  {"GET_MODULE_MEMORY_SIZE_MB", CommandCode::GET_MODULE_MEMORY_SIZE_MB},
  {"GET_MODULE_REVISION", CommandCode::GET_MODULE_REVISION},
  {"GET_MODULE_FORM_FACTOR", CommandCode::GET_MODULE_FORM_FACTOR},
  {"GET_MODULE_MEMORY_VENDOR_PART_NUMBER",
    CommandCode::GET_MODULE_MEMORY_VENDOR_PART_NUMBER},
  {"GET_MODULE_MEMORY_TYPE", CommandCode::GET_MODULE_MEMORY_TYPE},
  {"GET_FUSED_PUBLIC_KEYS", CommandCode::GET_FUSED_PUBLIC_KEYS}};

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

  std::unordered_map<uint32_t, std::shared_ptr<lockable_>[2]> deviceMap_;
};

typedef DeviceManagement &(*getDM_t)();

} // namespace device_management

#endif // ET_RUNTIME_DEVICEMANAGEMENT_H
