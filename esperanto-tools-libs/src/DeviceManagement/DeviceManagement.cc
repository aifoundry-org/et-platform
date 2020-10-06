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

bool DeviceManagement::isSetCommand(itCmd &cmd) {
  if (cmd->first.find("SET") == 0) {
    return true;
  }

  return false;
}

std::tuple<uint32_t, bool>
DeviceManagement::tokenizeDeviceNode(const char *device_node) {
  uint32_t index = 0;
  bool mgmtNode = false;

  std::string str(device_node);
  std::regex re("^et[0-9]+_(?=mgmt$|ops$)");
  std::smatch m;
  if (!std::regex_search(str, m, re)) {
    throw "Invalid device_node format!";
  }

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
  try {
    auto start = std::chrono::steady_clock::now();

    auto cmd = isValidCommand(cmd_code);
    if (cmd == commandCodeTable.end()) {
      throw "Error: Invalid command code!";
    }

    if (!device_node) {
      throw "Error: Invalid device_node pointer!";
    }

    auto isSet = isSetCommand(cmd);
    if (isSet && !input_buff) {
      throw "Error: Invalid input_buff pointer!";
    }

    if (!output_buff) {
      throw "Error: Invalid output_buff pointer!";
    }

    if (!host_latency_msec) {
      throw "Error: Invalid host_latency_msec pointer!";
    }

    if (!dev_latency_msec) {
      throw "Error: Invalid dev_latency_msec pointer!";
    }

    auto lockable = getDevice(tokenizeDeviceNode(device_node));

    if (lockable->devGuard.try_lock_for(
            std::chrono::milliseconds(timeout_msec))) {
      const std::lock_guard<std::timed_mutex> lock(lockable->devGuard,
                                                   std::adopt_lock_t());

      auto dmCB = std::make_unique<dmControlBlock>();
      dmCB->cmd_id = cmd_code;
      std::shared_ptr<char> payload;

      if (isSet && input_buff && input_size) {
        payload =
            std::allocate_shared<char>(std::allocator<char>(), input_size);
        memcpy(payload.get(), input_buff, input_size);
        memcpy(dmCB->cmd_payload, payload.get(), input_size);
      }

      if (!lockable->dev.init()) {
        throw "Unable to initialize device";
      }

      if (!lockable->dev.mb_write(dmCB.get(), input_size)) {
        throw "PCIeDevice did not successfully write";
      }

      if (!lockable->dev.mb_read(output_buff, output_size,
                                 std::chrono::milliseconds(timeout_msec))) {
        throw "PCIeDevice did not successfully read";
      }

      *dev_latency_msec = dmCB->dev_latency;
      *host_latency_msec = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - start).count();

      if (!lockable->dev.deinit()) {
        throw "Unable to deinitialize device";
      }

      return 0;
    }

    throw "Unable to acquire lock on device!";

  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (const char *msg) {
    std::cerr << msg << std::endl;
    return 1;
  }
}

extern "C" DeviceManagement &getInstance() {
  return DeviceManagement::getInstance();
}

} // namespace device_management
