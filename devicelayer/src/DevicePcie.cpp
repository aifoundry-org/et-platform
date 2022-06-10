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

template <typename... Types> IoctlResult wrap_ioctl(int fd, unsigned long int request, Types&&... args) {
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
DmaInfo DevicePcie::getDmaInfo(int device) const {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  DmaInfo dmaInfo;
  dmaInfo.maxElementSize_ = devices_[static_cast<unsigned long>(device)].userDram_.dma_max_elem_size;
  dmaInfo.maxElementCount_ = devices_[static_cast<unsigned long>(device)].userDram_.dma_max_elem_count;
  return dmaInfo;
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

    auto fd = mngmtEnabled_ ? deviceInfo.fdMgmt_ : deviceInfo.fdOps_;
    wrap_ioctl(fd, ETSOC1_IOCTL_GET_PCIBUS_DEVICE_NAME(deviceInfo.devName.size()), deviceInfo.devName.data());
    logs << "PCIBUS device name: " << deviceInfo.devName.data() << std::endl;

    dev_config cfg;
    wrap_ioctl(fd, ETSOC1_IOCTL_GET_DEVICE_CONFIGURATION, &cfg);
    deviceInfo.cfg_ = DeviceConfig{
      cfg.form_factor == DEV_CONFIG_FORM_FACTOR_PCIE ? DeviceConfig::FormFactor::PCIE : DeviceConfig::FormFactor::M2,
      cfg.tdp,
      cfg.total_l3_size,
      cfg.total_l2_size,
      cfg.total_scp_size,
      cfg.cache_line_size,
      4,                     /* SW-12534 REMOVE PLACEHOLDER (L2 cache banks) */
      128000,                /* SW-12534 REMOVE PLACEHOLDER (ddr bandwidth) */
      cfg.minion_boot_freq,
      cfg.cm_shire_mask,
      32,                    /* SW-12534 REMOVE PLACEHOLDER (spare minion shire id) */
      0                      /* SW-12534 REMOVE PLACEHOLDER (arch revision) */
    };

    logs << "TDP: " << deviceInfo.cfg_.tdp_ << "W\nMinion boot frequency: " << deviceInfo.cfg_.minionBootFrequency_
         << "MHz\nL3 size: " << deviceInfo.cfg_.totalL3Size_ << "MB\nL2 size: " << deviceInfo.cfg_.totalL2Size_
         << "MB\nScratch pad size: " << deviceInfo.cfg_.totalScratchPadSize_
         << "MB\nCache line size: " << deviceInfo.cfg_.cacheLineSize_ << "B\nCM active shire mask: 0x" << std::hex
         << deviceInfo.cfg_.computeMinionShireMask_ << std::endl;

    devices_.emplace_back(deviceInfo);
    DV_DLOG(DEBUG) << logs.str();
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
                                                std::chrono::milliseconds timeout) {
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
  auto readyEvents = epoll_wait(deviceInfo.epFdOps_, eventList, kMaxEpollEvents, static_cast<int>(timeout.count()));

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

size_t DevicePcie::getTraceBufferSizeMasterMinion(int device, TraceBufferType traceType) {
  if (!opsEnabled_) {
    throw Exception("Can't use Master Minion operations if master minion port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  const auto& deviceInfo = devices_[static_cast<unsigned long>(device)];

  auto trace_type = static_cast<uint8_t>(traceType);
  return static_cast<size_t>(wrap_ioctl(deviceInfo.fdOps_, ETSOC1_IOCTL_GET_TRACE_BUFFER_SIZE, &trace_type).rc_);
}

bool DevicePcie::sendCommandServiceProcessor(int device, std::byte* command, size_t commandSize, bool isMmReset) {
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
  if (isMmReset) {
    cmdInfo.flags |= CMD_DESC_FLAG_MM_RESET;
  }
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
                                                    std::chrono::milliseconds timeout) {
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

  readyEvents = epoll_wait(deviceInfo.epFdMgmt_, eventList, kMaxEpollEvents, static_cast<int>(timeout.count()));
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

bool DevicePcie::getTraceBufferServiceProcessor(int device, TraceBufferType traceType,
                                                std::vector<std::byte>& traceBuf) {

  if (!mngmtEnabled_) {
    throw Exception("Can't use Service Processor operations if service processor port is not enabled");
  }
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  auto& deviceInfo = devices_[static_cast<unsigned long>(device)];

  trace_desc traceInfo;
  traceInfo.trace_type = static_cast<uint8_t>(traceType);
  auto traceRegSize = wrap_ioctl(deviceInfo.fdMgmt_, ETSOC1_IOCTL_GET_TRACE_BUFFER_SIZE, &traceInfo.trace_type).rc_;

  traceBuf.resize(static_cast<uint64_t>(traceRegSize));
  traceInfo.buf = traceBuf.data();
  return wrap_ioctl(deviceInfo.fdMgmt_, ETSOC1_IOCTL_EXTRACT_TRACE_BUFFER, &traceInfo);
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

  if (auto freeMem = getCmaFreeMem(); sizeInBytes > freeMem) {
    throw Exception("Not enough CMA memory! (requested: " + std::to_string(sizeInBytes) +
                    ", available: " + std::to_string(freeMem) + ")");
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

int DevicePcie::getActiveShiresNum(int device) {
  DeviceConfig config = getDeviceConfig(device);
  uint32_t shireMask = config.computeMinionShireMask_;
  uint32_t checkPattern = 0x00000001;
  size_t maskBits = 32;
  int cntActive = 0;

  for (size_t i = 0; i < maskBits; i++) {
    bool active = shireMask & (checkPattern << i);
    if (active) {
      cntActive++;
    }
  }
  return cntActive;
}

uint32_t DevicePcie::getFrequencyMHz(int device) {
  DeviceConfig config = getDeviceConfig(device);
  return config.minionBootFrequency_;
}

std::string DevicePcie::getDeviceAttribute(int device, std::string relAttrPath) const {
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  fs::path absAttrPath = fs::path("/sys/bus/pci/devices/") /
                         fs::path(devices_[static_cast<uint32_t>(device)].devName.data()) / fs::path(relAttrPath);
  if (!fs::is_regular_file(absAttrPath)) {
    throw Exception("Invalid attribute file path: '" + absAttrPath.string() + "'");
  }
  std::ifstream file(absAttrPath.string());
  if (!file.is_open()) {
    throw Exception("Unable to access '" + absAttrPath.string() + "', try with sudo");
  }
  return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void DevicePcie::clearDeviceAttributes(int device, std::string relGroupPath) const {
  if (device >= static_cast<int>(devices_.size())) {
    throw Exception("Invalid device");
  }
  fs::path absGroupPath = fs::path("/sys/bus/pci/devices/") /
                          fs::path(devices_[static_cast<uint32_t>(device)].devName.data()) / fs::path(relGroupPath);
  if (!fs::is_directory(absGroupPath)) {
    throw Exception("Invalid attribute group directory: '" + absGroupPath.string() + "'");
  }
  fs::path clearFilePath = absGroupPath / fs::path("clear");
  if (!fs::is_regular_file(clearFilePath)) {
    throw Exception("Clear attribute file: '" + clearFilePath.string() + "' not found!");
  }
  std::ofstream file(clearFilePath.string());
  if (!file.is_open()) {
    throw Exception("Unable to access '" + clearFilePath.string() + "', try with sudo");
  }
  file << "1";
}
} // namespace dev
