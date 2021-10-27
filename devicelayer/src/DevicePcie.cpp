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
#include "Utils.h"
#include <cassert>
#include <cstring>
#include <dirent.h>
#include <experimental/filesystem>
#include <fcntl.h>
#include <fstream>
#include <mutex>
#include <regex>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

using namespace dev;
using namespace std::string_literals;
namespace fs = std::experimental::filesystem;

namespace {
int countDeviceNodes(bool isMngmt) {
  auto it = fs::directory_iterator("/dev");
  return static_cast<int>(std::count_if(fs::begin(it), fs::end(it), [isMngmt](auto& e) {
    return regex_match(e.path().filename().string(), std::regex(isMngmt ? "(et)(.*)(_mgmt)" : "(et)(.*)(_ops)"));
  }));
}

unsigned long getCmaFreeMem() {
  std::string token;
  std::ifstream file("/proc/meminfo");
  while (file >> token) {
    if (token == "CmaFree:") {
      unsigned long memInKB;
      if (file >> memInKB) {
        // Return mem in bytes
        return memInKB * 1024;
      } else {
        return 0;
      }
    }
    // Ignore the rest of the line
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  return 0;
}

struct IoctlResult {
  int rc_;
  operator bool() const {
    return rc_ >= 0;
  }
};

int openAndConfigEpoll(int fd) {
  int epFd = epoll_create(1);
  if (epFd < 0) {
    throw Exception("Error opening epoll file for fd: '" + std::to_string(fd) + "', '"s + std::strerror(errno) + "'"s);
  }

  epoll_event epEvent;
  epEvent.events = EPOLLIN | EPOLLOUT | EPOLLET;
  epEvent.data.fd = fd;

  if (epoll_ctl(epFd, EPOLL_CTL_ADD, fd, &epEvent) < 0) {
    throw Exception("Error setting up epoll for fd: '" + std::to_string(fd) + "', '"s + std::strerror(errno) + "'"s);
  }
  return epFd;
}

template <typename... Types> IoctlResult wrap_ioctl(int fd, unsigned long int request, Types... args) {
  std::stringstream params;
  ((params << ", " << std::forward<Types>(args)), ...);
  DV_VLOG(HIGH) << "Doing IOCTL fd: " << fd << " request: " << request << " args: " << params.str();
  auto res = ::ioctl(fd, request, args...);
  if (res < 0 && errno != EAGAIN) {
    DV_LOG(WARNING) << "IOCTL failed. FD: " << fd << " request: " << request << " args: " << params.str();
    throw Exception("Failed to execute IOCTL: '"s + std::strerror(errno) + "'"s);
  }
  return {res};
}

DeviceState getDeviceState(int fd) {
  uint32_t devState;
  wrap_ioctl(fd, ETSOC1_IOCTL_GET_DEVICE_STATE, &devState);
  DeviceState res;
  switch (devState) {
  case DEV_STATE_READY:
    res = DeviceState::Ready;
    break;
  case DEV_STATE_PENDING_COMMANDS:
    res = DeviceState::PendingCommands;
    break;
  case DEV_STATE_NOT_RESPONDING:
    res = DeviceState::NotResponding;
    break;
  default:
    res = DeviceState::Undefined;
  }
  return res;
}

constexpr int kMaxEpollEvents = 6;
} // namespace
namespace dev {

size_t DevicePcie::getFreeCmaMemory() const {
  return getCmaFreeMem();
}

int DevicePcie::getDmaAlignment() const {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  return devices_[0].userDram_.align_in_bits;
}

size_t DevicePcie::getDramSize() const {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  return devices_[0].userDram_.size;
}

uint64_t DevicePcie::getDramBaseAddress() const {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  return devices_[0].userDram_.base;
}

int DevicePcie::getDevicesCount() const {
  return static_cast<int>(devices_.size());
}

DeviceState DevicePcie::getDeviceStateMasterMinion(int device) const {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  return getDeviceState(devices_[static_cast<unsigned long>(device)].fdOps_);
}

DeviceState DevicePcie::getDeviceStateServiceProcessor(int device) const {
  if (!mngmtEnabled_) {
    throw Exception("Can't use Service Processor operations if service processor port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  return getDeviceState(devices_[static_cast<unsigned long>(device)].fdMgmt_);
}

int DevicePcie::getSubmissionQueuesCount(int device) const {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  return devices_[static_cast<unsigned long>(device)].mmSqCount_;
}

size_t DevicePcie::getSubmissionQueueSizeMasterMinion(int device) const {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  return devices_[static_cast<unsigned long>(device)].mmSqMaxMsgSize_;
}

size_t DevicePcie::getSubmissionQueueSizeServiceProcessor(int device) const {
  if (!mngmtEnabled_) {
    throw Exception("Can't use Service Processor operations if service processor port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  return devices_[static_cast<unsigned long>(device)].spSqMaxMsgSize_;
}

DevicePcie::DevicePcie(bool enableOps, bool enableMngmt)
  : opsEnabled_(enableOps)
  , mngmtEnabled_(enableMngmt) {
  if (!(enableOps || enableMngmt)) {
    throw Exception("Ops or Mngmt must be enabled");
  }

  int mngmtDevCount = countDeviceNodes(true);
  int opsDevCount = countDeviceNodes(false);

  // A device always has a mngmt node but ops node is optional.
  if (mngmtDevCount == 0) {
    throw Exception("No device available");
  }

  // If ops node does not exist then device is in recovery mode.
  if (opsDevCount != mngmtDevCount && enableOps) {
    throw Exception("Only Mngmt can be enabled in recovery mode");
  }

  for (int i = 0; i < mngmtDevCount; ++i) {
    DevInfo deviceInfo;
    std::stringstream logs;
    if (mngmtEnabled_) {
      char path[32];
      std::snprintf(path, sizeof(path), "/dev/et%d_mgmt", i);

      deviceInfo.fdMgmt_ = open(path, O_RDWR | O_NONBLOCK);
      if (deviceInfo.fdMgmt_ < 0) {
        throw Exception("Error opening mgmt file: '"s + std::strerror(errno) + "'"s);
      }

      wrap_ioctl(deviceInfo.fdMgmt_, ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE, &deviceInfo.spSqMaxMsgSize_);
      deviceInfo.epFdMgmt_ = openAndConfigEpoll(deviceInfo.fdMgmt_);

      logs << "\nPCIe target mgmt opened: \"" << path << "\""
           << "\nSP VQ Maximum message size: " << deviceInfo.spSqMaxMsgSize_ << std::endl;
    }
    if (opsEnabled_) {
      char path[32];
      std::snprintf(path, sizeof(path), "/dev/et%d_ops", i);
      deviceInfo.fdOps_ = open(path, O_RDWR | O_NONBLOCK);
      if (deviceInfo.fdOps_ < 0) {
        throw Exception("Error opening ops file: '"s + std::strerror(errno) + "'"s);
      }

      wrap_ioctl(deviceInfo.fdOps_, ETSOC1_IOCTL_GET_USER_DRAM_INFO, &deviceInfo.userDram_);
      wrap_ioctl(deviceInfo.fdOps_, ETSOC1_IOCTL_GET_SQ_COUNT, &deviceInfo.mmSqCount_);
      wrap_ioctl(deviceInfo.fdOps_, ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE, &deviceInfo.mmSqMaxMsgSize_);

      deviceInfo.epFdOps_ = openAndConfigEpoll(deviceInfo.fdOps_);

      logs << "\nPCIe target ops opened: \"" << path << "\""
           << "\nDRAM base: 0x" << std::hex << deviceInfo.userDram_.base << "\nDRAM size: 0x" << std::hex
           << deviceInfo.userDram_.size << "\nDRAM alignment: " << std::dec << deviceInfo.userDram_.align_in_bits
           << "bits"
           << "\nMM SQ count: " << deviceInfo.mmSqCount_
           << "\nMM VQ Maximum message size: " << deviceInfo.mmSqMaxMsgSize_ << std::endl;
    }

    dev_config cfg;
    wrap_ioctl(mngmtEnabled_ ? deviceInfo.fdMgmt_ : deviceInfo.fdOps_, ETSOC1_IOCTL_GET_DEVICE_CONFIGURATION, &cfg);
    deviceInfo.cfg_ = DeviceConfig{cfg.form_factor == DEV_CONFIG_FORM_FACTOR_PCIE ? DeviceConfig::FormFactor::PCIE
                                                                                  : DeviceConfig::FormFactor::M2,
                                   cfg.tdp,
                                   cfg.total_l3_size,
                                   cfg.total_l2_size,
                                   cfg.total_scp_size,
                                   cfg.cache_line_size,
                                   cfg.minion_boot_freq,
                                   cfg.cm_shire_mask};

    logs << "TDP: " << deviceInfo.cfg_.tdp_ << "W\nMinion boot frequency: " << deviceInfo.cfg_.minionBootFrequency_
         << "MHz\nL3 size: " << deviceInfo.cfg_.totalL3Size_ << "MB\nL2 size: " << deviceInfo.cfg_.totalL2Size_
         << "MB\nScratch pad size: " << deviceInfo.cfg_.totalScratchPadSize_
         << "MB\nCache line size: " << deviceInfo.cfg_.cacheLineSize_ << "B\nCM active shire mask: 0x" << std::hex
         << deviceInfo.cfg_.computeMinionShireMask_ << std::endl;

    devices_.emplace_back(deviceInfo);
    DV_LOG(INFO) << logs.str();
  }
}

DevicePcie::~DevicePcie() {
  for (auto& d : devices_) {
    if (opsEnabled_) {
      auto res = close(d.fdOps_);
      if (res < 0) {
        DV_LOG(FATAL) << "Failed to close ops file, error: " << std::strerror(errno);
      }
      res = close(d.epFdOps_);
      if (res < 0) {
        DV_LOG(FATAL) << "Failed to close ops epoll file, error: " << std::strerror(errno);
      }
    }
    if (mngmtEnabled_) {
      auto res = close(d.fdMgmt_);
      if (res < 0) {
        DV_LOG(FATAL) << "Failed to close mgmt file, error: " << std::strerror(errno);
      }

      res = close(d.epFdMgmt_);
      if (res < 0) {
        DV_LOG(FATAL) << "Failed to close mgmt epoll file, error: " << std::strerror(errno);
      }
    }
  }
}

bool DevicePcie::sendCommandMasterMinion(int device, int sqIdx, std::byte* command, size_t commandSize, bool isDma,
                                         bool isHighPriority) {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  auto& deviceInfo = devices_[static_cast<unsigned long>(device)];
  if (sqIdx >= deviceInfo.mmSqCount_) {
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
  if (isHighPriority) {
    cmdInfo.flags |= CMD_DESC_FLAG_HIGH_PRIORITY;
  }
  return wrap_ioctl(deviceInfo.fdOps_, ETSOC1_IOCTL_PUSH_SQ, &cmdInfo);
}

void DevicePcie::setSqThresholdMasterMinion(int device, int sqIdx, uint32_t bytesNeeded) {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  auto& deviceInfo = devices_[static_cast<unsigned long>(device)];
  if (sqIdx >= deviceInfo.mmSqCount_) {
    throw Exception("Invalid queue");
  }
  if (!bytesNeeded || bytesNeeded > deviceInfo.mmSqMaxMsgSize_) {
    throw Exception("Invalid value for bytesNeeded");
  }
  sq_threshold sqThreshold;
  sqThreshold.bytes_needed = static_cast<uint16_t>(bytesNeeded);
  sqThreshold.sq_index = static_cast<uint16_t>(sqIdx);
  wrap_ioctl(deviceInfo.fdOps_, ETSOC1_IOCTL_SET_SQ_THRESHOLD, &sqThreshold);
}

void DevicePcie::waitForEpollEventsMasterMinion(int device, uint64_t& sq_bitmap, bool& cq_available,
                                                std::chrono::seconds timeout) {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  auto& deviceInfo = devices_[static_cast<unsigned long>(device)];
  sq_bitmap = 0;
  cq_available = false;
  epoll_event eventList[kMaxEpollEvents];
  auto readyEvents =
    epoll_wait(deviceInfo.epFdOps_, eventList, kMaxEpollEvents,
               static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count()));

  if (readyEvents > 0) {
    for (int i = 0; i < readyEvents; i++) {
      if (eventList[i].events & (EPOLLOUT | EPOLLIN)) {
        if (eventList[i].events & EPOLLOUT) {
          wrap_ioctl(deviceInfo.fdOps_, ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP, &sq_bitmap);
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
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  auto& deviceInfo = devices_[static_cast<unsigned long>(device)];

  response.resize(deviceInfo.mmSqMaxMsgSize_);
  rsp_desc rspInfo;
  rspInfo.rsp = response.data();
  rspInfo.size = deviceInfo.mmSqMaxMsgSize_;
  rspInfo.cq_index = 0;
  return wrap_ioctl(deviceInfo.fdOps_, ETSOC1_IOCTL_POP_CQ, &rspInfo);
}

bool DevicePcie::sendCommandServiceProcessor(int device, std::byte* command, size_t commandSize) {
  if (!mngmtEnabled_) {
    throw Exception("Can't use Service Processor operations if service processor port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  auto& deviceInfo = devices_[static_cast<unsigned long>(device)];
  cmd_desc cmdInfo;
  cmdInfo.cmd = command;
  cmdInfo.size = static_cast<uint16_t>(commandSize);
  cmdInfo.sq_index = 0;
  cmdInfo.flags = 0;
  return wrap_ioctl(deviceInfo.fdMgmt_, ETSOC1_IOCTL_PUSH_SQ, &cmdInfo);
}

void DevicePcie::setSqThresholdServiceProcessor(int device, uint32_t bytesNeeded) {
  if (!mngmtEnabled_) {
    throw Exception("Can't use Service Processor operations if service processor port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  auto& deviceInfo = devices_[static_cast<unsigned long>(device)];
  if (!bytesNeeded || bytesNeeded > deviceInfo.mmSqMaxMsgSize_) {
    throw Exception("Invalid value for bytesNeeded");
  }
  sq_threshold sqThreshold;
  sqThreshold.bytes_needed = static_cast<uint16_t>(bytesNeeded);
  sqThreshold.sq_index = 0;
  wrap_ioctl(deviceInfo.fdMgmt_, ETSOC1_IOCTL_SET_SQ_THRESHOLD, &sqThreshold);
}

void DevicePcie::waitForEpollEventsServiceProcessor(int device, bool& sq_available, bool& cq_available,
                                                    std::chrono::seconds timeout) {
  if (!mngmtEnabled_) {
    throw Exception("Can't use Service Processor operations if service processor port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  auto& deviceInfo = devices_[static_cast<unsigned long>(device)];
  sq_available = false;
  cq_available = false;
  epoll_event eventList[kMaxEpollEvents];
  int readyEvents;

  readyEvents = epoll_wait(deviceInfo.epFdMgmt_, eventList, kMaxEpollEvents,
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
  if (!mngmtEnabled_) {
    throw Exception("Can't use Service Processor operations if service processor port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  auto& deviceInfo = devices_[static_cast<unsigned long>(device)];
  response.resize(deviceInfo.spSqMaxMsgSize_);
  rsp_desc rspInfo;
  rspInfo.rsp = response.data();
  rspInfo.size = deviceInfo.spSqMaxMsgSize_;
  rspInfo.cq_index = 0;
  return wrap_ioctl(deviceInfo.fdMgmt_, ETSOC1_IOCTL_POP_CQ, &rspInfo);
}

bool DevicePcie::getTraceBufferServiceProcessor(int device, TraceBufferType trace_type,
                                                std::vector<std::byte>& response) {

  if (!mngmtEnabled_) {
    throw Exception("Can't use Service Processor operations if service processor port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  auto& deviceInfo = devices_[static_cast<unsigned long>(device)];
  
  trace_desc traceInfo;
  traceInfo.trace_type = static_cast<uint8_t>(trace_type);
  auto traceRegSize =  wrap_ioctl(deviceInfo.fdMgmt_,
                                               ETSOC1_IOCTL_GET_DEVICE_MGMT_TRACE_BUFFER_SIZE,
                                               &traceInfo.trace_type).rc_;
  
  response.resize(traceRegSize);
  traceInfo.trace_type = static_cast<uint8_t>(trace_type);
  traceInfo.buf = response.data();
  return wrap_ioctl(deviceInfo.fdMgmt_, ETSOC1_IOCTL_EXTRACT_DEVICE_MGMT_TRACE_BUFFER, &traceInfo);
}

int DevicePcie::updateFirmwareImage(int device, std::vector<unsigned char>& fwImage) {
  if (!mngmtEnabled_) {
    throw Exception("Can't use Service Processor operations if service processor port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  const auto& deviceInfo = devices_[static_cast<unsigned long>(device)];
  fw_update_desc fwUpdateInfo;
  fwUpdateInfo.ubuf = fwImage.data();
  fwUpdateInfo.size = fwImage.size();
  return wrap_ioctl(deviceInfo.fdMgmt_, ETSOC1_IOCTL_FW_UPDATE, &fwUpdateInfo);
}

void* DevicePcie::allocDmaBuffer(int device, size_t sizeInBytes, bool writeable) {
  std::lock_guard lock(mutex_);
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }

  if (sizeInBytes > getCmaFreeMem()) {
    throw Exception("Not enough CMA memory!");
  }

  // NOTE fdOps_ must be open with O_RDWR for PROT_READ/PROT_WRITE with MAP_SHARED
  // Argument "prot" can be one of PROT_WRITE, PROT_READ, or both.
  // Refer: https://man7.org/linux/man-pages/man2/mmap.2.html
  auto res = mmap(nullptr, sizeInBytes, PROT_READ | (writeable ? PROT_WRITE : 0), MAP_SHARED,
                  devices_[static_cast<uint32_t>(device)].fdOps_, 0);

  if (res == MAP_FAILED) {
    throw Exception("Error mmap: '"s + std::strerror(errno) + "'");
  }
  dmaBuffers_[res] = sizeInBytes;
  return res;
}
void DevicePcie::freeDmaBuffer(void* dmaBuffer) {
  std::lock_guard lock(mutex_);
  auto it = dmaBuffers_.find(dmaBuffer);
  if (it == end(dmaBuffers_)) {
    throw Exception("Can't free a non previously allocated DmaBuffer");
  }
  if (munmap(dmaBuffer, it->second) != 0) {
    throw Exception("Error munmap: '"s + std::strerror(errno) + "'");
  }
  dmaBuffers_.erase(it);
}

DeviceConfig DevicePcie::getDeviceConfig(int device) {
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  return devices_[static_cast<uint32_t>(device)].cfg_;
}
} // namespace dev
