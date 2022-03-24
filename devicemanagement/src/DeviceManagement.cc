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

  ~lockable_() {
    if (receiverRunning) {
      receiverRunning = false;
    }
#if MINION_DEBUG_INTERFACE
    eventsCv.notify_all();
#endif
  }

  std::future<std::vector<std::byte>> getRespReceiveFuture(device_mgmt_api::tag_id_t tagId) {
    std::scoped_lock lk(mtx);
    std::promise<std::vector<std::byte>> p;
    commandMap.insert_or_assign(tagId, std::move(p));
    return commandMap[tagId].get_future();
  };

  bool fulfillRespReceivePromise(const std::vector<std::byte>& response) {
    auto tagId = reinterpret_cast<const dm_rsp*>(response.data())->info.rsp_hdr.tag_id;
    std::scoped_lock lk(mtx);
    if (commandMap.find(tagId) == commandMap.end()) {
      // An unkept promise cannot be fulfilled
      return false;
    }
    commandMap[tagId].set_value(response);
    return true;
  }

#if MINION_DEBUG_INTERFACE
  void pushEvent(std::vector<std::byte>& event) {
    std::scoped_lock lk(eventsMtx);
    events.push(std::move(event));
    eventsCv.notify_all();
  }

  bool popEvent(std::vector<std::byte>& event, uint32_t timeout) {
    std::unique_lock lk(eventsMtx);
    if (eventsCv.wait_for(lk, std::chrono::milliseconds(timeout), [this]() { return !events.empty(); })) {
      event = std::move(events.front());
      events.pop();
      return true;
    }
    return false;
  }
#endif

  uint32_t idx;
  std::atomic<bool> receiverRunning = false;
  std::thread receiver;
  std::timed_mutex sqGuard;
  std::timed_mutex cqGuard;

private:
  std::unordered_map<device_mgmt_api::tag_id_t, std::promise<std::vector<std::byte>>> commandMap;
  std::mutex mtx;
#if MINION_DEBUG_INTERFACE
  std::mutex eventsMtx;
  std::condition_variable eventsCv;
  std::queue<std::vector<std::byte>> events;
#endif
};

#if MINION_DEBUG_INTERFACE
bool DeviceManagement::handleEvent(const uint32_t device_node, std::vector<std::byte>& message) {
  auto rCB = reinterpret_cast<const dm_evt*>(message.data());
  // The range can be extended to handle all type of events
  if (rCB->info.event_hdr.msg_id >= device_mgmt_api::DM_CMD_MDI_SET_BREAKPOINT_EVENT &&
      rCB->info.event_hdr.msg_id < device_mgmt_api::DM_CMD_MDI_END) {
    auto lockable = getDevice(device_node);
    lockable->pushEvent(message);
    return true;
  }
  return false;
}

bool DeviceManagement::getEvent(const uint32_t device_node, std::vector<std::byte>& event,
                                uint32_t timeout) {
  auto lockable = getDevice(device_node);
  return lockable->popEvent(event, timeout);
}
#endif

void DeviceManagement::receiver(const uint32_t device_node) {
  auto lockable = getDevice(device_node);
  while (lockable->receiverRunning) {
    lockable->cqGuard.lock();
    std::vector<std::byte> message;
    while (devLayer_->receiveResponseServiceProcessor(lockable->idx, message)) {
#if MINION_DEBUG_INTERFACE
      if (handleEvent(lockable->idx, message)) {
        continue;
      }
#endif
      if (!lockable->fulfillRespReceivePromise(message)) {
        DV_DLOG(WARNING) << "Received a response for not sent command, discarding it.";
      }
    }
    lockable->cqGuard.unlock();
    auto sqAvail = false;
    auto cqAvail = false;
    while (lockable->receiverRunning && !cqAvail) {
      devLayer_->waitForEpollEventsServiceProcessor(lockable->idx, sqAvail, cqAvail);
    }
  }
}

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
      DV_DLOG(DEBUG) << "Command: " << it->first << " code: " << it->second << std::endl;
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
  static std::mutex mtx;
  std::scoped_lock lk(mtx);
  auto& ptr = deviceMap_[index];
  if (!ptr) {
    ptr = std::make_shared<lockable_>(index);
    if (!ptr->receiverRunning) {
      ptr->receiverRunning = true;
      ptr->receiver = std::thread(std::bind(&DeviceManagement::receiver, this, index));
      ptr->receiver.detach();
    }
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

int DeviceManagement::getTraceBufferServiceProcessor(const uint32_t device_node, TraceBufferType trace_type,
                                                     std::vector<std::byte>& response) {
  if (!isValidDeviceNode(device_node)) {
    return -EINVAL;
  }

  devLayer_->getTraceBufferServiceProcessor(device_node, trace_type, response);
  return 0;
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
  auto end = start + std::chrono::milliseconds(timeout);

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

  std::future<std::vector<std::byte>> respReceiveFuture;
  if (lockable->sqGuard.try_lock_for(end - std::chrono::steady_clock::now())) {
    const std::lock_guard<std::timed_mutex> lock(lockable->sqGuard, std::adopt_lock_t());

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

    respReceiveFuture = lockable->getRespReceiveFuture(wCB->info.cmd_hdr.tag_id);
    if (!devLayer_->sendCommandServiceProcessor(lockable->idx, reinterpret_cast<std::byte*>(wCB.get()),
                                                wCB->info.cmd_hdr.size)) {
      return -EIO;
    }

    DV_DLOG(DEBUG) << "Sent cmd: " << wCB->info.cmd_hdr.msg_id << " with header size: " << wCB->info.cmd_hdr.size
                   << std::endl;

    if (auto status = respReceiveFuture.wait_for(end - std::chrono::steady_clock::now());
        status != std::future_status::ready) {
      return -EAGAIN;
    }

    auto message = respReceiveFuture.get();
    auto rCB = reinterpret_cast<dm_rsp*>(message.data());

    DV_DLOG(DEBUG) << "Read rsp to cmd: " << rCB->info.rsp_hdr.msg_id << " with header size: " << rCB->info.rsp_hdr.size
                   << std::endl;

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

    return 0;
  }

  return -EAGAIN;
}

extern "C" DeviceManagement& getInstance(IDeviceLayer* devLayer) {
  return DeviceManagement::getInstance(devLayer);
}

} // namespace device_management
