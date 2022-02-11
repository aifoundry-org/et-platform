//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "deviceManagement/DeviceManagement.h"
#include "device-layer/IDeviceLayer.h"
#include "utils.h"

#include <cerrno>
#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <regex>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <vector>

using namespace dev;

namespace device_management {

struct lockable_ {
  explicit lockable_(uint32_t index)
    : idx(index) {
  }

  uint32_t idx;
  std::timed_mutex devGuard;
};

struct DeviceManagement::destruction_ {
  void operator()(const DeviceManagement* const ptr) {
    delete ptr;
  }
};

DeviceManagement& DeviceManagement::getInstance(IDeviceLayer* devLayer) {
  static std::unique_ptr<DeviceManagement, destruction_> instance(new DeviceManagement());
  (*instance).setDeviceLayer(devLayer);
  return *instance;
}

void DeviceManagement::setDeviceLayer(IDeviceLayer* ptr) {
  devLayer_ = ptr;
}

itCmd DeviceManagement::isValidCommand(uint32_t cmd_code) {
  for (auto it = commandCodeTable.begin(); it != commandCodeTable.end(); ++it) {
    if (it->second == cmd_code) {
      DV_LOG(INFO) << "Command: " << it->first << " code: " << it->second << std::endl;
      return it;
    }
  }

  return commandCodeTable.end();
}

bool DeviceManagement::isValidDeviceNode(const uint32_t device_node) {
  return device_node < devLayer_->getDevicesCount();
}

int DeviceManagement::getDevicesCount() {
  devLayer_->getDevicesCount();
}

bool DeviceManagement::isSetCommand(itCmd& cmd) {
  if (cmd->first.find("DM_CMD_SET") == 0) {
    return true;
  }

  return false;
}

bool DeviceManagement::isGetCommand(itCmd& cmd) {
  if (cmd->first.find("DM_CMD_GET") == 0) {
    return true;
  }

  return false;
}


std::shared_ptr<lockable_> DeviceManagement::getDevice(const uint32_t index) {
  auto& ptr = deviceMap_[index];

  if (!ptr) {
    ptr = std::make_shared<lockable_>(index);
  }

  return deviceMap_[index];
}

int DeviceManagement::updateFirmwareImage(std::shared_ptr<lockable_> lockable, const char* filePath) {

  std::ifstream file(filePath, std::ios::binary);

  if (!file.good()) {
    return -EINVAL;
  }

  std::vector<unsigned char> fwImage(std::istreambuf_iterator<char>(file), {});

  if (!devLayer_->updateFirmwareImage(lockable->idx, fwImage)) {
    DV_LOG(INFO) << "DeviceManagement::updateFirmwareImage failed" << std::endl;
    return -EIO;
  }

  DV_LOG(DEBUG) << "Written firmware image of size: " << fwImage.size() << std::endl;
  return 0;
}

bool DeviceManagement::isValidSHA512(const std::string& str) {
  std::regex re("^[[:xdigit:]]{128}$");
  std::smatch m;

  if (!std::regex_search(str, m, re)) {
    return false;
  }

  return true;
}

int DeviceManagement::processHashFile(const char* filePath, std::vector<unsigned char>& hash) {
  std::ifstream file(filePath);

  if (!file.good()) {
    return -EINVAL;
  }

  std::string contents("");
  std::copy_n(std::istreambuf_iterator<char>(file), 128, std::back_inserter(contents));

  if (!isValidSHA512(contents)) {
    return -EINVAL;
  }

  for (uint32_t i = 0; i < contents.size(); i += 2) {
    std::stringstream ss("");
    unsigned long value = 0;

    ss << contents[i] << contents[i + 1];

    if (ss.fail()) {
      return -EIO;
    }

    ss >> std::hex >> value;
    hash.emplace(hash.end(), value);
  }

  return 0;
}

int DeviceManagement::getTraceBufferServiceProcessor(const uint32_t device_node, TraceBufferType trace_type, std::vector<std::byte>& response,
                                                     uint32_t timeout) {
  if (!isValidDeviceNode(device_node)) {
    return -EINVAL;
  }

  auto lockable = getDevice(device_node);
  if (lockable->devGuard.try_lock_for(std::chrono::milliseconds(timeout))) {
    const std::lock_guard<std::timed_mutex> lock(lockable->devGuard, std::adopt_lock_t());
    devLayer_->getTraceBufferServiceProcessor(lockable->idx, trace_type, response);
    return 0;
  }

  return -EAGAIN;
}

bool DeviceManagement::isValidTdpLevel(const char* input_buff) {
  uint8_t tdp_level = input_buff[0];
  if (tdp_level < 40) {
    return true;
  }
  return false;
}

bool DeviceManagement::isValidActivePowerManagement(const char* input_buff) {
  for (auto it = activePowerManagementTable.begin(); it != activePowerManagementTable.end(); ++it) {
    if (it->second == *input_buff) {
      return true;
    }
  }
  return false;
}

bool DeviceManagement::isValidTemperature(const char* input_buff) {
  device_mgmt_api::temperature_threshold_t* temperature_threshold = (device_mgmt_api::temperature_threshold_t*)input_buff;
  if(temperature_threshold->sw_temperature_c >= 20 && temperature_threshold->sw_temperature_c <= 125) {
    return true;
  }
  return false;
}

bool DeviceManagement::isValidPcieLinkSpeed(const char* input_buff) {
  for (auto it = pcieLinkSpeedTable.begin(); it != pcieLinkSpeedTable.end(); ++it) {
    if (it->second == *input_buff) {
      return true;
    }
  }
  return false;
}

bool DeviceManagement::isValidPcieLaneWidth(const char* input_buff) {
  for (auto it = pcieLaneWidthTable.begin(); it != pcieLaneWidthTable.end(); ++it) {
    if (it->second == *input_buff) {
      return true;
    }
  }
  return false;
}

bool DeviceManagement::isInputBufferValid(uint32_t cmd_code, const char* input_buff) {
  bool ret;
  switch (cmd_code) {
    case device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_ACTIVE_POWER_MANAGEMENT:
      ret = isValidActivePowerManagement(input_buff);
      break;
    case device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_TEMPERATURE_THRESHOLDS:
      ret = isValidTemperature(input_buff);
      break;
    case device_mgmt_api::DM_CMD::DM_CMD_SET_MODULE_STATIC_TDP_LEVEL:
      ret = isValidTdpLevel(input_buff);
      break;
    case device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_MAX_LINK_SPEED:
      ret = isValidPcieLinkSpeed(input_buff);
      break;
    case device_mgmt_api::DM_CMD::DM_CMD_SET_PCIE_LANE_WIDTH:
      ret = isValidPcieLaneWidth(input_buff);
      break;
    default:
      ret = true;
      break;
  }
  return ret;
}

int DeviceManagement::serviceRequest(const uint32_t device_node, uint32_t cmd_code, const char* input_buff,
                                     const uint32_t input_size, char* output_buff, const uint32_t output_size,
                                     uint32_t* host_latency, uint64_t* dev_latency, uint32_t timeout) {

  auto start = std::chrono::steady_clock::now();

  if (!isValidDeviceNode(device_node)) {
    return -EINVAL;
  }

  auto cmd = isValidCommand(cmd_code);
  if (cmd == commandCodeTable.end()) {
    return -EINVAL;
  }

  auto isSet = isSetCommand(cmd);
  if (isSet && (!input_buff || !input_size)) {
    return -EINVAL;
  }

  auto isGet = isGetCommand(cmd);
  if (isGet && (!output_buff || !output_size)) {
    return -EINVAL;
  }

  if (!host_latency) {
    return -EINVAL;
  }

  if (!dev_latency) {
    return -EINVAL;
  }

  if (!devLayer_) {
    return -EINVAL;
  }

  if (!isInputBufferValid(cmd_code, input_buff)) {
    return -EINVAL;
  }

  auto inputSize = input_size;
  auto lockable = getDevice(device_node);

  if (lockable->devGuard.try_lock_for(std::chrono::milliseconds(timeout))) {
    const std::lock_guard<std::timed_mutex> lock(lockable->devGuard, std::adopt_lock_t());

    auto wCB = std::make_unique<dm_cmd>();
    wCB->info.cmd_hdr.tag_id = tag_id_++;
    wCB->info.cmd_hdr.msg_id = cmd_code;

    switch (cmd_code) {
    case device_mgmt_api::DM_CMD::DM_CMD_SET_FIRMWARE_UPDATE: {
      wCB->info.cmd_hdr.size = sizeof(wCB->info);
      DV_LOG(DEBUG) << "Size: " << std::dec << wCB->info.cmd_hdr.size;
      int res = updateFirmwareImage(lockable, input_buff);

      if (res != 0) {
        DV_LOG(INFO) << "failed to write firmware image to device DRAM";
        return res;
      }
    } break;
    case device_mgmt_api::DM_CMD::DM_CMD_SET_SP_BOOT_ROOT_CERT:
    case device_mgmt_api::DM_CMD::DM_CMD_SET_SW_BOOT_ROOT_CERT: {
      std::vector<unsigned char> hash;

      int res = processHashFile(input_buff, hash);

      if (res != 0) {
        DV_LOG(INFO) << "process hash file error ";
        return res;
      }
      inputSize = hash.size();
      if (sizeof(wCB->payload) < inputSize) {
        DV_LOG(INFO) << "not enough space in cmd payload ";
        return -EAGAIN;
      }
      auto tmp = reinterpret_cast<char*>(hash.data());
      DV_LOG(INFO) << "Mem copy ";
      memcpy(wCB->payload, tmp, inputSize);
      DV_LOG(INFO) << "Size: " << inputSize;
      wCB->info.cmd_hdr.size = (sizeof(*(wCB.get())) - 1) + inputSize;
      DV_LOG(INFO) << "input_buff: " << tmp;
    } break;
    case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES:
    case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_RESIDENCY_POWER_STATES:
    case device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_RUN_CONTROL:
    case device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_CONFIG:
#if MINION_DEBUG_INTERFACE    
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_SELECT_HART:
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_UNSELECT_HART:
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_RESET_HART:
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_HALT_HART:
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_RESUME_HART:
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_GET_HART_STATUS:
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_SET_BREAKPOINT:
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_UNSET_BREAKPOINT:
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_ENABLE_SINGLE_STEP:
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_DISABLE_SINGLE_STEP:
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_GPR:
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_DUMP_GPR:
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_WRITE_GPR:
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_CSR:
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_WRITE_CSR:
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_READ_MEM:
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_WRITE_MEM:
#endif 
    {
      memcpy(wCB->payload, input_buff, inputSize);
      wCB->info.cmd_hdr.size = (sizeof(*(wCB.get())) - 1) + inputSize;
      break;
    }
    default: {
      if (isSet && input_buff && inputSize) {
        memcpy(wCB->payload, input_buff, inputSize);
      }

      wCB->info.cmd_hdr.size = (inputSize) ? (sizeof(*(wCB.get())) - 1) + inputSize : sizeof(*(wCB.get()));
    } break;
    }

    if (!devLayer_->sendCommandServiceProcessor(lockable->idx, reinterpret_cast<std::byte*>(wCB.get()),
                                                wCB->info.cmd_hdr.size)) {
      return -EIO;
    }

    DV_LOG(INFO) << "Sent cmd: " << wCB->info.cmd_hdr.msg_id << " with header size: " << wCB->info.cmd_hdr.size
                 << std::endl;

    int responseReceived = 0;
    bool sq_available = true, cq_available = true, event_pending = true;
    while (responseReceived < 1) {
      if (start + std::chrono::milliseconds(timeout) < std::chrono::steady_clock::now()) {
        DV_LOG(INFO) << "Timeout expired while waiting for rsp to cmd: " << wCB->info.cmd_hdr.msg_id << std::endl;
        return -EAGAIN;
      }

      if (event_pending) {
        event_pending = false;
      } else {
        devLayer_->waitForEpollEventsServiceProcessor(lockable->idx, sq_available, cq_available);
      }

      if (cq_available) {
        std::vector<std::byte> message;
        if (!devLayer_->receiveResponseServiceProcessor(lockable->idx, message)) {
          continue;
        }
        auto rCB = reinterpret_cast<dm_rsp*>(message.data());

        if (rCB->info.rsp_hdr.msg_id != wCB->info.cmd_hdr.msg_id ||
            rCB->info.rsp_hdr.tag_id != wCB->info.cmd_hdr.tag_id) {
          DV_LOG(INFO) << "Read rsp to cmd: " << rCB->info.rsp_hdr.msg_id << " (tag_id: " << rCB->info.rsp_hdr.tag_id
                       << ") but expected: " << wCB->info.cmd_hdr.msg_id << " (tag_id: " << wCB->info.cmd_hdr.tag_id
                       << ")" << std::endl;
          // Discarding if old/different response is received
          event_pending = true;
          continue;
        }

        DV_LOG(INFO) << "Read rsp to cmd: " << rCB->info.rsp_hdr.msg_id
                     << " with header size: " << rCB->info.rsp_hdr.size << std::endl;

        if (output_buff && output_size) {
          memcpy(output_buff, rCB->payload, output_size);
        }

        auto status = rCB->info.rsp_hdr_ext.status;

        if (status) {
          DV_LOG(INFO) << "Received incorrect rsp status: " << rCB->info.rsp_hdr_ext.status << std::endl;
          return -EIO;
        }

        *dev_latency = rCB->info.rsp_hdr_ext.device_latency_usec;
        *host_latency =
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

        responseReceived++;

        return 0;
      }
    }
  }

  return -EAGAIN;
}

extern "C" DeviceManagement& getInstance(IDeviceLayer* devLayer) {
  return DeviceManagement::getInstance(devLayer);
}

} // namespace device_management
