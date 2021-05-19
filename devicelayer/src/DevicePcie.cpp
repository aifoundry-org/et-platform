/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "DevicePcie.h"
#include "utils.h"
#include <cassert>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <regex>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <unistd.h>

using namespace dev;
using namespace std::string_literals;

namespace dev {
namespace {
struct IoctlResult {
  int rc_;
  operator bool() const {
    return rc_ >= 0;
  }
};

template <typename... Types> IoctlResult wrap_ioctl(int fd, unsigned long int request, Types... args) {
  std::stringstream ss;
  ss << "Doing IOCTL fd: " << fd << " request: " << request << " args: ";
  ((ss << ", " << std::forward<Types>(args)), ...);
  DV_VLOG(HIGH) << ss.str();
  auto res = ::ioctl(fd, request, args...);
  if (res < 0 && errno != EAGAIN) {
    throw Exception("Failed to execute IOCTL: '"s + std::strerror(errno) + "'"s);
  }
  return {res};
}

constexpr int kMaxEpollEvents = 6;
} // namespace
} // namespace dev

int DevicePcie::getDmaAlignment() const {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  auto it = devices_.find(0);
  if (it != devices_.end()) {
    return it->second.userDram.align_in_bits;
  } else {
    throw Exception("Invalid device");
  }
}

size_t DevicePcie::getDramSize() const {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  auto it = devices_.find(0);
  if (it != devices_.end()) {
    return it->second.userDram.size;
  } else {
    throw Exception("Invalid device");
  }
}

uint64_t DevicePcie::getDramBaseAddress() const {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  auto it = devices_.find(0);
  if (it != devices_.end()) {
    return it->second.userDram.base;
  } else {
    throw Exception("Invalid device");
  }
}

int DevicePcie::getDevicesCount() const {
  return static_cast<int>(devices_.size());
}

int DevicePcie::getSubmissionQueuesCount(int device) const {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  auto it = devices_.find(device);
  if (it != devices_.end()) {
    return it->second.mmSqCount;
  } else {
    throw Exception("Invalid device");
  }
}

size_t DevicePcie::getSubmissionQueueSizeMasterMinion(int device) const {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  auto it = devices_.find(device);
  if (it != devices_.end()) {
    return it->second.mmSqMaxMsgSize;
  } else {
    throw Exception("Invalid device");
  }
}

size_t DevicePcie::getSubmissionQueueSizeServiceProcessor(int device) const {
  if (!mngmtEnabled_) {
    throw Exception("Can't use Service Processor operations if service processor port is not enabled");
  }
  auto it = devices_.find(device);
  if (it != devices_.end()) {
    return it->second.spSqMaxMsgSize;
  } else {
    throw Exception("Invalid device");
  }
}

int DevicePcie::countDeviceNodes() {
  dirent* entry;
  DIR* dir = opendir("/dev");
  int count = 0;

  if (dir == NULL) {
    throw Exception("Unable to open directory");
  }
  while ((entry = readdir(dir)) != NULL) {
    if (regex_match(entry->d_name, std::regex("(et)(.*)(_ops)"))) {
      count++;
    }
  }
  closedir(dir);
  return count;
}

DevicePcie::DevicePcie(bool enableOps, bool enableMngmt)
  : opsEnabled_(enableOps)
  , mngmtEnabled_(enableMngmt) {
  LOG_IF(FATAL, !(enableOps || enableMngmt)) << "Ops or Mngmt must be enabled";

  devInfo deviceInfo;
  int devCount = DevicePcie::countDeviceNodes();

  for (int i = 0; i < devCount; i++) {
    deviceInfo.devIdx = i;
    if (mngmtEnabled_) {
      char path[32];
      std::snprintf(path, sizeof(path), "/dev/et%d_mgmt", deviceInfo.devIdx);

      deviceInfo.fdMgmt = open(path, O_RDWR | O_NONBLOCK);
      if (deviceInfo.fdMgmt < 0) {
        throw Exception("Error opening mgmt file: '"s + std::strerror(errno) + "'"s);
      }
      DV_LOG(INFO) << "PCIe target mgmt opened: \"" << path << "\"";

      wrap_ioctl(deviceInfo.fdMgmt, ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE, &deviceInfo.spSqMaxMsgSize);
      DV_LOG(INFO) << "SP VQ Maximum message size: " << deviceInfo.spSqMaxMsgSize;

      deviceInfo.epFdMgmt = epoll_create(1);
      if (deviceInfo.epFdMgmt < 0) {
        throw Exception("Error opening epoll file for mgmt: '"s + std::strerror(errno) + "'"s);
      }

      epoll_event epEvent;
      epEvent.events = EPOLLIN | EPOLLOUT | EPOLLET;
      epEvent.data.fd = deviceInfo.fdMgmt;

      if (epoll_ctl(deviceInfo.epFdMgmt, EPOLL_CTL_ADD, deviceInfo.fdMgmt, &epEvent) < 0) {
        throw Exception("Error setting up epoll on mgmt file: '"s + std::strerror(errno) + "'"s);
      }
    }
    if (opsEnabled_) {
      char path[32];
      std::snprintf(path, sizeof(path), "/dev/et%d_ops", deviceInfo.devIdx);
      deviceInfo.fdOps = open(path, O_RDWR | O_NONBLOCK);
      if (deviceInfo.fdOps < 0) {
        throw Exception("Error opening ops file: '"s + std::strerror(errno) + "'"s);
      }
      DV_LOG(INFO) << "PCIe target ops opened: \"" << path << "\"";

      wrap_ioctl(deviceInfo.fdOps, ETSOC1_IOCTL_GET_USER_DRAM_INFO, &deviceInfo.userDram);
      DV_LOG(INFO) << "DRAM base: 0x" << std::hex << deviceInfo.userDram.base;
      DV_LOG(INFO) << "DRAM size: 0x" << std::hex << deviceInfo.userDram.size;
      DV_LOG(INFO) << "DRAM alignment: " << deviceInfo.userDram.align_in_bits << "bits";

      wrap_ioctl(deviceInfo.fdOps, ETSOC1_IOCTL_GET_SQ_COUNT, &deviceInfo.mmSqCount);
      DV_LOG(INFO) << "MM SQ count: " << deviceInfo.mmSqCount;

      wrap_ioctl(deviceInfo.fdOps, ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE, &deviceInfo.mmSqMaxMsgSize);
      DV_LOG(INFO) << "MM VQ Maximum message size: " << deviceInfo.mmSqMaxMsgSize << "\n";

      deviceInfo.epFdOps = epoll_create(1);
      if (deviceInfo.epFdOps < 0) {
        throw Exception("Error opening epoll file for ops: '"s + std::strerror(errno) + "'"s);
      }

      epoll_event epEvent;
      epEvent.events = EPOLLIN | EPOLLOUT | EPOLLET;
      epEvent.data.fd = deviceInfo.fdOps;

      if (epoll_ctl(deviceInfo.epFdOps, EPOLL_CTL_ADD, deviceInfo.fdOps, &epEvent) < 0) {
        throw Exception("Error setting up epoll on ops file: '"s + std::strerror(errno) + "'"s);
      }
    }
    devices_.emplace(i, std::move(deviceInfo));
  }
}

DevicePcie::~DevicePcie() {
  devInfo deviceInfo;
  int devCount = DevicePcie::countDeviceNodes();

  for (int i = 0; i < devCount; i++) {
    auto it = devices_.find(i);
    if (it == devices_.end()) {
      DV_LOG(FATAL) << "Unable to destroy device[" << i << "]";
      continue;
    }
    deviceInfo = it->second;

    if (opsEnabled_) {
      auto res = close(deviceInfo.fdOps);
      if (res < 0) {
        DV_LOG(FATAL) << "Failed to close ops file, error: " << std::strerror(errno);
      }

      res = close(deviceInfo.epFdOps);
      if (res < 0) {
        DV_LOG(FATAL) << "Failed to close ops epoll file, error: " << std::strerror(errno);
      }
    }
    if (mngmtEnabled_) {
      auto res = close(deviceInfo.fdMgmt);
      if (res < 0) {
        DV_LOG(FATAL) << "Failed to close mgmt file, error: " << std::strerror(errno);
      }

      res = close(deviceInfo.epFdMgmt);
      if (res < 0) {
        DV_LOG(FATAL) << "Failed to close mgmt epoll file, error: " << std::strerror(errno);
      }
    }
  }
  devices_.clear();
}

bool DevicePcie::sendCommandMasterMinion(int device, int sqIdx, std::byte* command, size_t commandSize, bool isDma) {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  auto it = devices_.find(device);
  if (it == devices_.end()) {
    throw Exception("Invalid device");
  }
  auto deviceInfo = it->second;
  if (sqIdx >= deviceInfo.mmSqCount) {
    throw Exception("Invalid queue");
  }
  cmd_desc cmdInfo;
  cmdInfo.cmd = command;
  cmdInfo.size = static_cast<uint16_t>(commandSize);
  cmdInfo.sq_index = static_cast<uint16_t>(sqIdx);
  cmdInfo.flags = 0;
  if (isDma) {
    cmdInfo.flags |= CMD_DESC_FLAG_DMA;
  }
  return wrap_ioctl(deviceInfo.fdOps, ETSOC1_IOCTL_PUSH_SQ, &cmdInfo);
}

void DevicePcie::setSqThresholdMasterMinion(int device, int sqIdx, uint32_t bytesNeeded) {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  auto it = devices_.find(device);
  if (it == devices_.end()) {
    throw Exception("Invalid device");
  }
  auto deviceInfo = it->second;
  if (sqIdx >= deviceInfo.mmSqCount) {
    throw Exception("Invalid queue");
  }
  if (!bytesNeeded || bytesNeeded > deviceInfo.mmSqMaxMsgSize) {
    throw Exception("Invalid value for bytesNeeded");
  }
  sq_threshold sqThreshold;
  sqThreshold.bytes_needed = static_cast<uint16_t>(bytesNeeded);
  sqThreshold.sq_index = static_cast<uint16_t>(sqIdx);
  wrap_ioctl(deviceInfo.fdOps, ETSOC1_IOCTL_SET_SQ_THRESHOLD, &sqThreshold);
}

void DevicePcie::waitForEpollEventsMasterMinion(int device, uint64_t& sq_bitmap, bool& cq_available,
                                                std::chrono::seconds timeout) {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  auto it = devices_.find(device);
  if (it == devices_.end()) {
    throw Exception("Invalid device");
  }
  auto deviceInfo = it->second;
  sq_bitmap = 0;
  cq_available = false;
  epoll_event eventList[kMaxEpollEvents];
  auto readyEvents =
    epoll_wait(deviceInfo.epFdOps, eventList, kMaxEpollEvents,
               static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count()));

  if (readyEvents > 0) {
    for (int i = 0; i < readyEvents; i++) {
      if (eventList[i].events & (EPOLLOUT | EPOLLIN)) {
        if (eventList[i].events & EPOLLOUT) {
          wrap_ioctl(deviceInfo.fdOps, ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP, &sq_bitmap);
        }
        if (eventList[i].events & EPOLLIN) {
          cq_available = true;
        }
      } else {
        throw Exception("Unknown epoll event");
      }
    }
  } else if (readyEvents < 0) {
    throw Exception("epoll_wait() failed: '"s + std::strerror(errno) + "'"s);
  }
}

bool DevicePcie::receiveResponseMasterMinion(int device, std::vector<std::byte>& response) {
  // TODO: Optimize it to use only the maximum size of device-api messages
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  auto it = devices_.find(device);
  if (it == devices_.end()) {
    throw Exception("Invalid device");
  }
  auto deviceInfo = it->second;

  response.resize(deviceInfo.mmSqMaxMsgSize);
  rsp_desc rspInfo;
  rspInfo.rsp = response.data();
  rspInfo.size = deviceInfo.mmSqMaxMsgSize;
  rspInfo.cq_index = 0;
  return wrap_ioctl(deviceInfo.fdOps, ETSOC1_IOCTL_POP_CQ, &rspInfo);
}

bool DevicePcie::sendCommandServiceProcessor(int device, std::byte* command, size_t commandSize) {
  if (!mngmtEnabled_) {
    throw Exception("Can't use Service Processor operations if service processor port is not enabled");
  }
  auto it = devices_.find(device);
  if (it == devices_.end()) {
    throw Exception("Invalid device");
  }
  auto deviceInfo = it->second;
  cmd_desc cmdInfo;
  cmdInfo.cmd = command;
  cmdInfo.size = static_cast<uint16_t>(commandSize);
  cmdInfo.sq_index = 0;
  cmdInfo.flags = 0;
  return wrap_ioctl(deviceInfo.fdMgmt, ETSOC1_IOCTL_PUSH_SQ, &cmdInfo);
}

void DevicePcie::setSqThresholdServiceProcessor(int device, uint32_t bytesNeeded) {
  if (!mngmtEnabled_) {
    throw Exception("Can't use Service Processor operations if service processor port is not enabled");
  }
  auto it = devices_.find(device);
  if (it == devices_.end()) {
    throw Exception("Invalid device");
  }
  auto deviceInfo = it->second;
  if (!bytesNeeded || bytesNeeded > deviceInfo.mmSqMaxMsgSize) {
    throw Exception("Invalid value for bytesNeeded");
  }
  sq_threshold sqThreshold;
  sqThreshold.bytes_needed = static_cast<uint16_t>(bytesNeeded);
  sqThreshold.sq_index = 0;
  wrap_ioctl(deviceInfo.fdMgmt, ETSOC1_IOCTL_SET_SQ_THRESHOLD, &sqThreshold);
}

void DevicePcie::waitForEpollEventsServiceProcessor(int device, bool& sq_available, bool& cq_available,
                                                    std::chrono::seconds timeout) {
  if (!mngmtEnabled_) {
    throw Exception("Can't use Service Processor operations if service processor port is not enabled");
  }
  auto it = devices_.find(device);
  if (it == devices_.end()) {
    throw Exception("Invalid device");
  }
  auto deviceInfo = it->second;
  sq_available = false;
  cq_available = false;
  epoll_event eventList[kMaxEpollEvents];
  int readyEvents;

  readyEvents = epoll_wait(deviceInfo.epFdMgmt, eventList, kMaxEpollEvents,
                           static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count()));
  if (readyEvents > 0) {
    for (int i = 0; i < readyEvents; i++) {
      if (eventList[i].events & (EPOLLOUT | EPOLLIN)) {
        if (eventList[i].events & EPOLLOUT) {
          sq_available = true;
        }
        if (eventList[i].events & EPOLLIN) {
          cq_available = true;
        }
      } else {
        throw Exception("Unknown epoll event");
      }
    }
  } else if (readyEvents < 0) {
    throw Exception("epoll_wait() failed: '"s + std::strerror(errno) + "'"s);
  }
}

bool DevicePcie::receiveResponseServiceProcessor(int device, std::vector<std::byte>& response) {
  // TODO: Optimize it to use only the maximum size of device-api messages
  if (!mngmtEnabled_) {
    throw Exception("Can't use Service Processor operations if service processor port is not enabled");
  }
  auto it = devices_.find(device);
  if (it == devices_.end()) {
    throw Exception("Invalid device");
  }
  auto deviceInfo = it->second;
  response.resize(deviceInfo.spSqMaxMsgSize);
  rsp_desc rspInfo;
  rspInfo.rsp = response.data();
  rspInfo.size = deviceInfo.spSqMaxMsgSize;
  rspInfo.cq_index = 0;
  return wrap_ioctl(deviceInfo.fdMgmt, ETSOC1_IOCTL_POP_CQ, &rspInfo);
}
