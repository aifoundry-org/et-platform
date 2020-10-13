//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/DeviceManagement/DeviceManagement.h"
#include "PCIEDevice/PCIeDevice.h"

#include <errno.h>
#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <regex>
#include <tuple>
#include <unordered_map>

namespace device_management {

struct lockable_ {
  explicit lockable_(uint32_t index, bool mgmtNode = false)
      : dev(index, mgmtNode) {}
  et_runtime::device::PCIeDevice dev;
  std::timed_mutex devGuard;
};

struct DeviceManagement::destruction_ {
  void operator()(const DeviceManagement *const ptr) { delete ptr; }
};

DeviceManagement &DeviceManagement::getInstance() {
  static std::unique_ptr<DeviceManagement, destruction_> instance(
      new DeviceManagement());
  return *instance;
}

itCmd DeviceManagement::isValidCommand(uint32_t cmd_code) {
  for (auto it = commandCodeTable.begin(); it != commandCodeTable.end(); ++it) {
    if (it->second == cmd_code) {
      return it;
    }
  }

  return commandCodeTable.end();
}

bool DeviceManagement::isValidDeviceNode(const char *device_node) {
  std::string str(device_node);
  std::regex re("^et[0-5]{1}_(?=mgmt$|ops$)");
  std::smatch m;

  if (!std::regex_search(str, m, re)) {
    return false;
  }

  return true;
}

bool DeviceManagement::isSetCommand(itCmd &cmd) {
  if (cmd->first.find("SET") == 0) {
    return true;
  }

  return false;
}

std::tuple<uint32_t, bool>
DeviceManagement::tokenizeDeviceNode(const char *device_node) {
  std::string str(device_node);
  uint32_t index = 0;
  bool mgmtNode = false;

  str = str.substr(2);
  std::size_t pos = str.find("_");

  index = static_cast<uint32_t>(std::stoul(str.substr(0, pos)));
  mgmtNode = (str.substr(pos + 1) == "mgmt") ? true : false;

  return {index, mgmtNode};
}

std::shared_ptr<lockable_>
DeviceManagement::getDevice(const std::tuple<uint32_t, bool> &t) {
  uint32_t index = std::get<0>(t);
  bool mgmtNode = std::get<1>(t);
  auto &ptr = deviceMap_[index][mgmtNode];

  if (!ptr) {
    ptr = std::make_shared<lockable_>(index, mgmtNode);
  }

  return deviceMap_[index][mgmtNode];
}

int DeviceManagement::serviceRequest(
    const char *device_node, uint32_t cmd_code, const char *input_buff,
    const uint32_t input_size, char *output_buff, const uint32_t output_size,
    uint32_t *host_latency_msec, uint32_t *dev_latency_msec,
    uint32_t timeout_msec) {

  auto start = std::chrono::steady_clock::now();

  if (!isValidDeviceNode(device_node)) {
    return -EINVAL;
  }

  auto cmd = isValidCommand(cmd_code);
  if (cmd == commandCodeTable.end()) {
    return -EINVAL;
  }

  auto isSet = isSetCommand(cmd);
  if (isSet && !input_buff) {
    return -EINVAL;
  }

  if (!output_buff) {
    return -EINVAL;
  }

  if (!host_latency_msec) {
    return -EINVAL;
  }

  if (!dev_latency_msec) {
    return -EINVAL;
  }

  auto lockable = getDevice(tokenizeDeviceNode(device_node));

  if (lockable->devGuard.try_lock_for(
          std::chrono::milliseconds(timeout_msec))) {
    const std::lock_guard<std::timed_mutex> lock(lockable->devGuard,
                                                  std::adopt_lock_t());

    auto wCB = std::make_unique<dmControlBlock>();
    wCB->cmd_id = cmd_code;
    std::shared_ptr<char> wPayload;

    if (isSet && input_buff && input_size) {
      wPayload =
          std::allocate_shared<char>(std::allocator<char>(), input_size);
      memcpy(wPayload.get(), input_buff, input_size);
      memcpy(wCB->cmd_payload, wPayload.get(), input_size);
    }

    if (!lockable->dev.init()) {
      return -EIO;
    }

    if (!lockable->dev.mb_write(wCB.get(), sizeof(*(wCB.get())) + input_size)) {
      return -EIO;
    }

    auto rCB = std::make_unique<dmControlBlock>();
    std::shared_ptr<char> rPayload;

    rPayload =
          std::allocate_shared<char>(std::allocator<char>(), output_size);
    memcpy(rCB->cmd_payload, rPayload.get(), output_size);

    if (!lockable->dev.mb_read(rCB.get(), sizeof(*(rCB.get())) + output_size,
                                std::chrono::milliseconds(timeout_msec))) {
      return -EIO;
    }

    memcpy(output_buff, rCB->cmd_payload, output_size);

    *dev_latency_msec = rCB->dev_latency;
    *host_latency_msec = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - start).count();

    if (!lockable->dev.deinit()) {
      return -EIO;
    }

    return 0;
  }

  return -EAGAIN;
}

extern "C" DeviceManagement &getInstance() {
  return DeviceManagement::getInstance();
}

} // namespace device_management
