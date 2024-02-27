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
#include <regex>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <vector>

/* Maximum DM events for fixed queue*/
#define MAX_DM_EVENTS 100

using namespace dev;

namespace device_management {

template <typename T, int MaxLen, typename Container = std::deque<T>>
class FixedQueue : public std::queue<T, Container> {
public:
  void push(const T& value) {
    if (this->size() == MaxLen) {
      this->c.pop_front();
    }
    std::queue<T, Container>::push(value);
  }
};

struct lockable_ {
  explicit lockable_(uint32_t index)
    : idx(index) {
    exitReceiverFuture = exitReceiverPromise.get_future();
  }

  ~lockable_() {
    if (receiverRunning) {
      receiverRunning = false;
    }
    eventsCv.notify_all();
  }

  std::future<std::vector<std::byte>> getRespReceiveFuture(device_mgmt_api::tag_id_t tagId) {
    std::scoped_lock lk(commandMapMtx);
    std::promise<std::vector<std::byte>> p;
    commandMap.insert_or_assign(tagId, std::move(p));
    return commandMap[tagId].get_future();
  };

  bool fulfillRespReceivePromise(const std::vector<std::byte>& response) {
    auto tagId = reinterpret_cast<const dm_rsp*>(response.data())->info.rsp_hdr.tag_id;
    std::scoped_lock lk(commandMapMtx);
    if (auto it = commandMap.find(tagId); it != commandMap.end()) {
      it->second.set_value(response);
      commandMap.erase(it);
      return true;
    }
    auto rsp = reinterpret_cast<const dm_rsp*>(response.data());
    DV_LOG(FATAL) << "Stray response received: tag_id: " << rsp->info.rsp_hdr.tag_id
                  << ", msg_id: " << rsp->info.rsp_hdr.msg_id;
    // An unkept promise cannot be fulfilled
    return false;
  }

  void pushEvent(std::vector<std::byte>& event) {
    std::scoped_lock lk(eventsMtx);
    auto rCB = reinterpret_cast<const dm_evt*>(event.data());
    if (rCB->info.event_hdr.msg_id >= device_mgmt_api::DM_CMD_MDI_SET_BREAKPOINT_EVENT &&
        rCB->info.event_hdr.msg_id < device_mgmt_api::DM_CMD_MDI_END) {
      mdiEvents.push(std::move(event));
    } else if (rCB->info.event_hdr.msg_id >= device_mgmt_api::DM_EVENT_SP_TRACE_BUFFER_FULL &&
               rCB->info.event_hdr.msg_id < device_mgmt_api::DM_EVENT_END) {
      dmEvents.push(std::move(event));
    }
    eventsCv.notify_all();
  }

  bool popMDIEvent(std::vector<std::byte>& event, uint32_t timeout) {
    std::unique_lock lk(eventsMtx);
    if (eventsCv.wait_for(lk, std::chrono::milliseconds(timeout),
                          [this]() { return !receiverRunning || !mdiEvents.empty(); })) {
      if (!mdiEvents.empty()) {
        event = std::move(mdiEvents.front());
        mdiEvents.pop();
        return true;
      }
    }
    return false;
  }

  bool popDMEvent(std::vector<std::byte>& event, uint32_t timeout) {
    std::unique_lock lk(eventsMtx);
    if (eventsCv.wait_for(lk, std::chrono::milliseconds(timeout),
                          [this]() { return !receiverRunning || !dmEvents.empty(); })) {
      if (!dmEvents.empty()) {
        event = std::move(dmEvents.front());
        dmEvents.pop();
        return true;
      }
    }
    return false;
  }

  uint32_t idx;
  // receiver is run in detach mode so no need to wait for receiver thread to join
  // But for some scenarios when it is required to know the completion of receiver
  // thread exitReceiverFuture can be used.
  std::thread receiver;
  std::atomic<bool> receiverRunning = false;
  std::shared_future<void> exitReceiverFuture;
  std::promise<void> exitReceiverPromise;
  std::timed_mutex sqGuard;
  std::timed_mutex cqGuard;

private:
  std::unordered_map<device_mgmt_api::tag_id_t, std::promise<std::vector<std::byte>>> commandMap;
  std::mutex commandMapMtx;
  std::mutex eventsMtx;
  std::condition_variable eventsCv;
  std::queue<std::vector<std::byte>> mdiEvents;
  FixedQueue<std::vector<std::byte>, MAX_DM_EVENTS> dmEvents;
};

bool DeviceManagement::handleEvent(const uint32_t device_node, std::vector<std::byte>& message) {
  auto rCB = reinterpret_cast<const dm_evt*>(message.data());
  // The range can be extended to handle all type of events
  if ((rCB->info.event_hdr.msg_id >= device_mgmt_api::DM_CMD_MDI_SET_BREAKPOINT_EVENT &&
       rCB->info.event_hdr.msg_id < device_mgmt_api::DM_CMD_MDI_END) ||
      (rCB->info.event_hdr.msg_id >= device_mgmt_api::DM_EVENT_BEGIN &&
       rCB->info.event_hdr.msg_id < device_mgmt_api::DM_EVENT_END)) {

    auto lockable = getDeviceInstance(device_node);
    lockable->pushEvent(message);
    return true;
  }
  return false;
}

bool DeviceManagement::getMDIEvent(const uint32_t device_node, std::vector<std::byte>& event, uint32_t timeout) {
  auto lockable = getDeviceInstance(device_node);
  return lockable->popMDIEvent(event, timeout);
}

/* TODO: SW-18541 - This API is deprecated, use getMDIEvent() instead */
bool DeviceManagement::getEvent(const uint32_t device_node, std::vector<std::byte>& event, uint32_t timeout) {
  return getMDIEvent(device_node, event, timeout);
}

bool DeviceManagement::getDMEvent(const uint32_t device_node, std::vector<std::byte>& event, uint32_t timeout) {
  auto lockable = getDeviceInstance(device_node);
  return lockable->popDMEvent(event, timeout);
}

void DeviceManagement::receiver(std::shared_ptr<lockable_> lockable) {
  while (lockable->receiverRunning) {
    lockable->cqGuard.lock();
    std::vector<std::byte> message;
    while (devLayer_->receiveResponseServiceProcessor(lockable->idx, message)) {
      if (handleEvent(lockable->idx, message)) {
        continue;
      }
      if (!lockable->fulfillRespReceivePromise(message)) {
        DV_DLOG(WARNING) << "receiver: discarding the stray message";
      }
    }
    lockable->cqGuard.unlock();
    auto sqAvail = false;
    auto cqAvail = false;
    while (lockable->receiverRunning && !cqAvail) {
      try {
        devLayer_->waitForEpollEventsServiceProcessor(lockable->idx, sqAvail, cqAvail, std::chrono::seconds(1));
      } catch (const dev::Exception& ex) {
        if (std::string(ex.what()).find(std::strerror(EINTR)) != std::string::npos) {
          // Ignore 'Interrupted system call' error since this thread is running
          // in detach mode and epoll call can be interrupted during tear-down.
          continue;
        }
        auto eptr = std::make_exception_ptr(ex);
        std::rethrow_exception(eptr);
      }
    }
  }
  lockable->exitReceiverPromise.set_value_at_thread_exit();
}

struct DeviceManagement::destruction_ {
  void operator()(const DeviceManagement* const ptr) {
    delete ptr;
  }
};

DeviceManagement& DeviceManagement::getInstance(IDeviceLayer* devLayer) {
  static std::unique_ptr<DeviceManagement, destruction_> instance(new DeviceManagement());
  (*instance).setDeviceLayer(devLayer);
  for (int i = 0; i < devLayer->getDevicesCount(); i++) {
    (*instance).createDeviceInstance(static_cast<unsigned int>(i));
  }
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

void DeviceManagement::createDeviceInstance(const uint32_t device_node) {
  std::scoped_lock lk(deviceMapMtx_);
  if (auto& ptr = deviceMap_[device_node]; !ptr) {
    ptr = std::make_shared<lockable_>(device_node);
    if (!ptr->receiverRunning) {
      ptr->receiverRunning = true;
      ptr->receiver = std::thread(std::bind(&DeviceManagement::receiver, this, ptr));
      ptr->receiver.detach();
    }
  }
}

void DeviceManagement::destroyDeviceInstance(const uint32_t device_node, bool waitReceiverExit) {
  std::scoped_lock lk(deviceMapMtx_);
  if (auto& ptr = deviceMap_[device_node]; ptr) {
    ptr->receiverRunning = false;
    if (waitReceiverExit) {
      ptr->exitReceiverFuture.get();
    }
  }
  deviceMap_.erase(device_node);
}

void DeviceManagement::destroyDevicesInstance(bool waitReceiverExit) {
  std::scoped_lock lk(deviceMapMtx_);
  for (auto& d : deviceMap_) {
    if (auto& ptr = d.second; ptr) {
      ptr->receiverRunning = false;
    }
  }
  if (waitReceiverExit) {
    for (auto& d : deviceMap_) {
      auto& ptr = d.second;
      if (auto& ptr = d.second; ptr) {
        ptr->exitReceiverFuture.get();
      }
    }
  }
  deviceMap_.clear();
}

std::shared_ptr<lockable_> DeviceManagement::getDeviceInstance(const uint32_t index) {
  std::scoped_lock lk(deviceMapMtx_);
  auto& ptr = deviceMap_[index];
  if (!ptr) {
    throw Exception("Device[" + std::to_string(index) + "] does not exist!");
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

bool DeviceManagement::isValidActivePowerManagement(const char* input_buff) {
  for (auto it = activePowerManagementTable.begin(); it != activePowerManagementTable.end(); ++it) {
    if (it->second == *input_buff) {
      return true;
    }
  }
  return false;
}

bool DeviceManagement::isValidTemperature(const char* input_buff) {
  device_mgmt_api::temperature_threshold_t* temperature_threshold =
    (device_mgmt_api::temperature_threshold_t*)input_buff;
  if (temperature_threshold->sw_temperature_c >= 20 && temperature_threshold->sw_temperature_c <= 125) {
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
  auto lockable = getDeviceInstance(device_node);

  std::future<std::vector<std::byte>> respReceiveFuture;
  if (lockable->sqGuard.try_lock_for(end - std::chrono::steady_clock::now())) {
    const std::lock_guard<std::timed_mutex> lock(lockable->sqGuard, std::adopt_lock_t());

    auto wCB = std::make_unique<dm_cmd>();
    wCB->info.cmd_hdr.tag_id = tag_id_++;
    wCB->info.cmd_hdr.msg_id = cmd_code;
    wCB->payload = std::make_unique<char[]>(inputSize);

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
      memcpy(wCB->payload.get(), input_buff, inputSize);
      DV_LOG(INFO) << "Size: " << inputSize;
      wCB->info.cmd_hdr.size = sizeof(wCB->info) + inputSize;
      DV_LOG(INFO) << "input_buff: " << tmp;
    } break;

    case device_mgmt_api::DM_CMD::DM_CMD_SET_FRU:{
      //memcpy(wCB->payload,input_buff,inputSize);
      //wCB->info.cmd_hdr.size=sizeof(wCB->info)+inputSize;
    } break;
    case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_RESIDENCY_THROTTLE_STATES:
    case device_mgmt_api::DM_CMD::DM_CMD_GET_MODULE_RESIDENCY_POWER_STATES:
    case device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_RUN_CONTROL:
    case device_mgmt_api::DM_CMD::DM_CMD_SET_DM_TRACE_CONFIG:
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
    case device_mgmt_api::DM_CMD::DM_CMD_MDI_WRITE_MEM: {
      memcpy(wCB->payload.get(), input_buff, inputSize);
      wCB->info.cmd_hdr.size = sizeof(wCB->info) + inputSize;
      break;
    }
    default: {
      if (isSet && input_buff && inputSize) {
        memcpy(wCB->payload.get(), input_buff, inputSize);
      }
      wCB->info.cmd_hdr.size = sizeof(wCB->info) + inputSize;
    } break;
    }
    CmdFlagSP flags;
    if (cmd_code == device_mgmt_api::DM_CMD::DM_CMD_MM_RESET) {
      flags.isMmReset_ = true;
    } else if (cmd_code == device_mgmt_api::DM_CMD::DM_CMD_RESET_ETSOC) {
      flags.isEtsocReset_ = true;
    }
    // There will be no response for ETSOC Reset, so do not add response placeholder and stop device functionality
    if (flags.isEtsocReset_) {
      destroyDeviceInstance(lockable->idx, true);
    } else {
      respReceiveFuture = lockable->getRespReceiveFuture(wCB->info.cmd_hdr.tag_id);
    }
    size_t totalSize = sizeof(wCB->info) + input_size;
    auto buffer = std::make_unique<std::byte[]>(totalSize);
    memcpy(buffer.get(), &(wCB->info), sizeof(wCB->info));
    memcpy(buffer.get() + sizeof(wCB->info), wCB->payload.get(), input_size);
    try {
      if (!devLayer_->sendCommandServiceProcessor(lockable->idx, buffer.get(), wCB->info.cmd_hdr.size, flags)) {
        if (flags.isEtsocReset_) {
          createDeviceInstance(lockable->idx);
        }

        return -EIO;
      }
    } catch (const dev::Exception& ex) {
      auto eptr = std::make_exception_ptr(ex);
      std::rethrow_exception(eptr);
    }
    DV_DLOG(DEBUG) << "Sent cmd: " << wCB->info.cmd_hdr.msg_id << " with header size: " << wCB->info.cmd_hdr.size
                   << std::endl;

    if (flags.isEtsocReset_) {
      devLayer_->reinitDeviceInstance(
        lockable->idx, false,
        std::chrono::duration_cast<std::chrono::milliseconds>(end - std::chrono::steady_clock::now()));
      createDeviceInstance(lockable->idx);
      return 0;
    }
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
