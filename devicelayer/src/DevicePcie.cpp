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
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cassert>
#include <stdio.h>
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>
#include <unistd.h>

using namespace dev;
using namespace std::string_literals;

namespace dev {
namespace {
struct IoctlResult {
  int rc_;
  operator bool() const {return rc_ >= 0;}
};

template<typename ...Types>
IoctlResult wrap_ioctl(int fd, unsigned long int request, Types... args) {
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
constexpr int kNumDevices = 1;
}
}

int DevicePcie::getDmaAlignment() const {
  return userDram_.align_in_bits;
}

size_t DevicePcie::getDramSize() const {
  return userDram_.size;
}

uint64_t DevicePcie::getDramBaseAddress() const {
  return userDram_.base;
}

int DevicePcie::getDevicesCount() const {
  // TODO: SW-5934: Multiple devices handling in deviceLayer
  return kNumDevices;
}

int DevicePcie::getSubmissionQueuesCount(int device) const {
  return mmSqCount_;
}

size_t DevicePcie::getSubmissionQueueSizeMasterMinion(int device) const {
  // Size of all submission queues is same.
  return mmSqMaxMsgSize_;
}

size_t DevicePcie::getSubmissionQueueSizeServiceProcessor(int device) const {
  return spSqMaxMsgSize_;
}

DevicePcie::DevicePcie() {
  // TODO: SW-5934: Multiple devices handling in deviceLayer
  // Currently device 0 is initialized only
  devIdx_ = 0;

  char path[32];
  std::snprintf(path, sizeof(path), "/dev/et%d_mgmt", devIdx_);

  fdMgmt_ = open(path, O_RDWR | O_NONBLOCK);
  if (fdMgmt_ < 0) {
    throw Exception("Error opening mgmt file: '"s + std::strerror(errno) + "'"s);
  }
  DV_LOG(INFO) << "PCIe target mgmt opened: \"" << path << "\"\n";

  wrap_ioctl(fdMgmt_, ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE, &spSqMaxMsgSize_);
  DV_LOG(INFO) << "SP VQ Maximum message size: " << spSqMaxMsgSize_ << "\n";

  std::snprintf(path, sizeof(path), "/dev/et%d_ops", devIdx_);
  fdOps_ = open(path, O_RDWR | O_NONBLOCK);
  if (fdOps_ < 0) {
    throw Exception("Error opening ops file: '"s + std::strerror(errno) + "'"s);
  }
  DV_LOG(INFO) << "PCIe target ops opened: \"" << path << "\"\n";

  wrap_ioctl(fdOps_, ETSOC1_IOCTL_GET_USER_DRAM_INFO, &userDram_);
  DV_LOG(INFO) << "DRAM base: 0x" << std::hex << userDram_.base << "\n";
  DV_LOG(INFO) << "DRAM size: 0x" << std::hex << userDram_.size << "\n";
  DV_LOG(INFO) << "DRAM alignment: " << userDram_.align_in_bits << "bits" << "\n";

  wrap_ioctl(fdOps_, ETSOC1_IOCTL_GET_SQ_COUNT, &mmSqCount_);
  DV_LOG(INFO) << "MM SQ count: " << mmSqCount_ << "\n";

  wrap_ioctl(fdOps_, ETSOC1_IOCTL_GET_SQ_MAX_MSG_SIZE, &mmSqMaxMsgSize_);
  DV_LOG(INFO) << "MM VQ Maximum message size: " << mmSqMaxMsgSize_ << "\n";

  epFdOps_ = epoll_create(1);
  if (epFdOps_ < 0) {
    throw Exception("Error opening epoll file for ops: '"s + std::strerror(errno) + "'"s);
  }

  epoll_event epEvent;
  epEvent.events = EPOLLIN | EPOLLOUT | EPOLLET;
  epEvent.data.fd = fdOps_;

  if (epoll_ctl(epFdOps_, EPOLL_CTL_ADD, fdOps_, &epEvent) < 0) {
    throw Exception("Error setting up epoll on ops file: '"s + std::strerror(errno) + "'"s);
  }

  epFdMgmt_ = epoll_create(1);
  if (epFdMgmt_ < 0) {
    throw Exception("Error opening epoll file for mgmt: '"s + std::strerror(errno) + "'"s);
  }

  epEvent.data.fd = fdMgmt_;

  if (epoll_ctl(epFdMgmt_, EPOLL_CTL_ADD, fdMgmt_, &epEvent) < 0) {
    throw Exception("Error setting up epoll on mgmt file: '"s + std::strerror(errno) + "'"s);
  }
}

DevicePcie::~DevicePcie() {
  auto res = close(fdOps_);
  if (res < 0) {
    DV_LOG(FATAL) << "Failed to close ops file, error: " << std::strerror(errno) << "\n";
  }

  res = close(epFdOps_);
  if (res < 0) {
    DV_LOG(FATAL) << "Failed to close ops epoll file, error: " << std::strerror(errno) << "\n";
  }

  res = close(fdMgmt_);
  if (res < 0) {
    DV_LOG(FATAL) << "Failed to close mgmt file, error: " << std::strerror(errno) << "\n";
  }

  res = close(epFdMgmt_);
  if (res < 0) {
    DV_LOG(FATAL) << "Failed to close mgmt epoll file, error: " << std::strerror(errno) << "\n";
  }
}

bool DevicePcie::sendCommandMasterMinion(int device, int sqIdx, std::byte* command, size_t commandSize) {
  if (sqIdx >= mmSqCount_) {
    throw Exception("Invalid queue");
  }
  cmd_desc cmdInfo;
  cmdInfo.cmd = command;
  cmdInfo.size = commandSize;
  cmdInfo.sq_index = sqIdx;
  cmdInfo.flags = 0;
  auto header = reinterpret_cast<device_ops_api::cmn_header_t*>(command);
  if (header->msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD ||
      header->msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD) {
    cmdInfo.flags |= CMD_DESC_FLAG_DMA;
  }
  return wrap_ioctl(fdOps_, ETSOC1_IOCTL_PUSH_SQ, &cmdInfo);
}

void DevicePcie::setSqThresholdMasterMinion(int device, int sqIdx, uint32_t bytesNeeded) {
  if (sqIdx >= mmSqCount_) {
    throw Exception("Invalid queue");
  }
  if (!bytesNeeded || bytesNeeded > mmSqMaxMsgSize_) {
    throw Exception("Invalid value for bytesNeeded");
  }
  sq_threshold sqThreshold;
  sqThreshold.bytes_needed = bytesNeeded;
  sqThreshold.sq_index = sqIdx;
  wrap_ioctl(fdOps_, ETSOC1_IOCTL_SET_SQ_THRESHOLD, &sqThreshold);
}

void DevicePcie::waitForEpollEventsMasterMinion(int device, uint64_t& sq_bitmap, bool& cq_available,
                                                std::chrono::seconds timeout) {
  sq_bitmap = 0;
  cq_available = false;
  epoll_event eventList[kMaxEpollEvents];
  auto readyEvents = epoll_wait(epFdOps_, eventList, kMaxEpollEvents,
                                std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());

  if (readyEvents > 0) {
    for (int i = 0; i < readyEvents; i++) {
      if (eventList[i].events & (EPOLLOUT | EPOLLIN)) {
        if (eventList[i].events & EPOLLOUT) {
          wrap_ioctl(fdOps_, ETSOC1_IOCTL_GET_SQ_AVAIL_BITMAP, &sq_bitmap);
        }
        if (eventList[i].events & EPOLLIN) {
          cq_available = true;
        }
      } else {
        throw Exception("Unknown epoll event");
      }
    }
  }
  else if (readyEvents < 0) {
    throw Exception("epoll_wait() failed: '"s + std::strerror(errno) + "'"s);
  }
}

bool DevicePcie::receiveResponseMasterMinion(int device, std::vector<std::byte>& response) {
  // TODO: Optimize it to use only the maximum size of device-api messages
  response.resize(mmSqMaxMsgSize_);
  int dataRead = 0;
  rsp_desc rspInfo;
  rspInfo.rsp = response.data();
  rspInfo.size = mmSqMaxMsgSize_;
  rspInfo.cq_index = 0;
  return wrap_ioctl(fdOps_, ETSOC1_IOCTL_POP_CQ, &rspInfo);
}

bool DevicePcie::sendCommandServiceProcessor(int device, std::byte* command, size_t commandSize) {
  cmd_desc cmdInfo;
  cmdInfo.cmd = command;
  cmdInfo.size = commandSize;
  cmdInfo.sq_index = 0;
  cmdInfo.flags = 0;
  return wrap_ioctl(fdMgmt_, ETSOC1_IOCTL_PUSH_SQ, &cmdInfo);
}

void DevicePcie::setSqThresholdServiceProcessor(int device, uint32_t bytesNeeded) {
  if (!bytesNeeded || bytesNeeded > mmSqMaxMsgSize_) {
    throw Exception("Invalid value for bytesNeeded");
  }
  sq_threshold sqThreshold;
  sqThreshold.bytes_needed = bytesNeeded;
  sqThreshold.sq_index = 0;
  wrap_ioctl(fdMgmt_, ETSOC1_IOCTL_SET_SQ_THRESHOLD, &sqThreshold);
}

void DevicePcie::waitForEpollEventsServiceProcessor(int device, bool& sq_available, bool& cq_available,
                                                    std::chrono::seconds timeout) {
  sq_available = false;
  cq_available = false;
  epoll_event eventList[kMaxEpollEvents];
  int readyEvents;

  readyEvents = epoll_wait(epFdMgmt_, eventList, kMaxEpollEvents,
                           std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
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
  }
  else if (readyEvents < 0) {
    throw Exception("epoll_wait() failed: '"s + std::strerror(errno) + "'"s);
  }
}

bool DevicePcie::receiveResponseServiceProcessor(int device, std::vector<std::byte>& response) {
  // TODO: Optimize it to use only the maximum size of device-api messages
  response.resize(spSqMaxMsgSize_);
  int dataRead = 0;
  rsp_desc rspInfo;
  rspInfo.rsp = response.data();
  rspInfo.size = spSqMaxMsgSize_;
  rspInfo.cq_index = 0;
  return wrap_ioctl(fdMgmt_, ETSOC1_IOCTL_POP_CQ, &rspInfo);
}
