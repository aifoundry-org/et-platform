/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "DeviceSysEmu.h"
#include "SysEmuHostListener.h"
#include "Utils.h"
#include <boost/crc.hpp>
#include <chrono>
#include <elfio/elfio.hpp>
#include <experimental/filesystem>
#include <future>
#include <mutex>
#include <stdio.h>
#include <thread>

namespace fs = std::experimental::filesystem;

using namespace dev;
using namespace std::string_literals;

namespace {
enum VERBOSITY { LOW = 0, MID = 5, HIGH = 10 };
using namespace std::chrono_literals;

// TODO. These magic numbers should be configurable?
constexpr auto kVirtualQueuesDiscoveryTimeout = 300s;
constexpr auto kPollingInterval = 100ms;
constexpr int kDmaAlignment = 64;
constexpr int kNumDevices = 1;
// this value comes from device-api sizeof(rsp_header_t). should always be kept in sync with this file:
// https://gitlab.esperanto.ai/software/device-api/-/blob/master/src/device-apis/device_apis_message_types.h
constexpr auto kCommonHeaderSize = 8U;
// This is the max possible size of the device API commands (DMA list with all 4 entries)
constexpr auto kMMThresholdBytes = 136U;

constexpr auto kDmaElemSize = 64 << 20;
constexpr auto kDmaElemCount = 4;

constexpr size_t getAvailSpace(const CircBuffCb& buffer) {
  auto head = buffer.head_offset;
  auto tail = buffer.tail_offset;
  auto length = buffer.length;
  if (head >= tail) {
    return (length - 1) - (head - tail);
  } else {
    return tail - head - 1;
  }
}
constexpr size_t getUsedSpace(const CircBuffCb& buffer) {
  auto head = buffer.head_offset;
  auto tail = buffer.tail_offset;
  auto length = buffer.length;
  if (head >= tail) {
    return (head - tail);
  } else {
    return length + head - tail;
  }
}
} // namespace

DmaInfo DeviceSysEmu::getDmaInfo(int) const {
  DmaInfo dmaInfo;
  dmaInfo.maxElementCount_ = kDmaElemCount;
  dmaInfo.maxElementSize_ = kDmaElemSize;
  return dmaInfo;
}

int DeviceSysEmu::getDmaAlignment() const {
  return kDmaAlignment;
}

uint64_t DeviceSysEmu::getDramSize() const {
  return mmInfo_.mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_OPS_HOST_MANAGED].bar_size;
}

uint64_t DeviceSysEmu::getDramBaseAddress() const {
  return mmInfo_.mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_OPS_HOST_MANAGED].dev_address;
}

int DeviceSysEmu::getDevicesCount() const {
  return kNumDevices;
}

int DeviceSysEmu::getSubmissionQueuesCount(int) const {
  return mmInfo_.vq_attr.sq_count;
}

DeviceState DeviceSysEmu::getDeviceStateMasterMinion(int) const {
  return DeviceState::Ready;
}

DeviceState DeviceSysEmu::getDeviceStateServiceProcessor(int) const {
  return DeviceState::Ready;
}

size_t DeviceSysEmu::getSubmissionQueueSizeMasterMinion(int) const {
  // Submission queue size discovered from DIRs is sum of actual circular buffer and
  // it's header. User of this function doesn't need to know the underlying details
  // and should get the buffer size.
  // Size of all submission queues is same.
  return submissionQueuesMM_[0].size_ - sizeof(CircBuffCb);
}

size_t DeviceSysEmu::getSubmissionQueueSizeServiceProcessor(int) const {
  // Submission queue size discovered from DIRs is sum of actual circular buffer and
  // it's header. User of this function doesn't need to know the underlying details
  // and should get the buffer size.
  return submissionQueueSP_.size_ - sizeof(CircBuffCb);
}

size_t DeviceSysEmu::getTraceBufferSizeMasterMinion(int, TraceBufferType traceType) {
  size_t traceBufSize = 0;
  if (traceType == TraceBufferType::TraceBufferMM) {
    traceBufSize = spInfo_.mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_MMFW_TRACE].bar_size;
  } else if (traceType == TraceBufferType::TraceBufferCM) {
    traceBufSize = spInfo_.mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_MNGT_CMFW_TRACE].bar_size;
  } else {
    throw Exception("Unsupported trace type!");
  }
  return traceBufSize;
}

// Stub only since SysEmu is to be deprecated
bool DeviceSysEmu::getTraceBufferServiceProcessor(int, TraceBufferType, std::vector<std::byte>&) {
  return false;
}

// Stub only since SysEmu is to be deprecated
int DeviceSysEmu::updateFirmwareImage(int, std::vector<unsigned char>&) {
  return 0;
}

DeviceSysEmu::DeviceSysEmu(const emu::SysEmuOptions& options) {
  // Selected by the BIOS
  barAddress_[0] = 0x1000000000;
  barAddress_[2] = 0x2000000000;

  DV_DLOG(INFO) << "BAR0 selected: 0x" << std::hex << barAddress_[0];
  DV_DLOG(INFO) << "BAR2 selected: 0x" << std::hex << barAddress_[2];

  // TODO: Add VQ offsets to FW types
  spDevIntfRegAddr_ = barAddress_[2] + 0x1000;
  mmDevIntfRegAddr_ = barAddress_[2] + 0x0UL;

  sysEmu_ = emu::ISysEmu::create(options, barAddress_, &hostListener_);

  hostListener_.getPcieReadyFuture().get();
  interruptListener_ = std::thread([&] {
    while (isRunning_) {
      // Device Management:
      //  - MSI Vector[0] - Mgmt SQ
      //  - MSI Vector[1] - Mgmt CQ
      // Device Operations:
      //  - MSI Vector[2] - Ops SQ(s)
      //  - MSI Vector[3] - Ops CQ
      auto bitmap = sysEmu_->waitForInterrupt(SP_SQ | SP_CQ | MM_SQ | MM_CQ);
      std::lock_guard lock(mutex_);
      if (bitmap & (SP_SQ | SP_CQ)) {
        spIntrptBitmap_ |= bitmap & (SP_SQ | SP_CQ);
        spEpollBlock_.notify_all();
      }
      if (bitmap & (MM_SQ | MM_CQ)) {
        mmIntrptBitmap_ |= bitmap & (MM_SQ | MM_CQ);
        mmEpollBlock_.notify_all();
      }
    }
  });

  setupServiceProcessor();
  setupMasterMinion();
}

DeviceSysEmu::~DeviceSysEmu() {
  DV_LOG(INFO) << "Destroying DeviceSysEmu. Sysemu ptr: " << sysEmu_.get();
  isRunning_ = false;
  DV_LOG(INFO) << "Stopping sysemu.";
  sysEmu_.reset();
  DV_LOG(INFO) << "Checking last errors from sysemu.";
  checkSysemuLastError(); // this needs to be done after sysemu instance has been destroyed
  DV_LOG(INFO) << "Awake all waiting threads.";
  spEpollBlock_.notify_all();
  mmEpollBlock_.notify_all();
  DV_LOG(INFO) << "Joining interrupt listener.";
  interruptListener_.join();
  DV_LOG(INFO) << "Interrupt listener joined";
}

bool DeviceSysEmu::sendCommand(QueueInfo& queueInfo, std::byte* command, size_t commandSize, bool& clearEvent) {
  Checker checker{*this};
  clearEvent = true;

  // read tail_offset
  sysEmu_->mmioRead(queueInfo.bufferAddress_ + offsetof(CircBuffCb, tail_offset), sizeof(queueInfo.cb_.tail_offset),
                    reinterpret_cast<std::byte*>(&queueInfo.cb_.tail_offset));

  // check if there is enough space
  if (getAvailSpace(queueInfo.cb_) < commandSize) {
    return false;
  }

  // Check if buffer wrap is required
  if (queueInfo.cb_.head_offset + commandSize > queueInfo.cb_.length) {
    auto bytesUntilEnd = queueInfo.cb_.length - queueInfo.cb_.head_offset;
    sysEmu_->mmioWrite(queueInfo.bufferAddress_ + sizeof(CircBuffCb) + queueInfo.cb_.head_offset, bytesUntilEnd,
                       command);
    queueInfo.cb_.head_offset = 0;
    commandSize -= bytesUntilEnd;
    command += bytesUntilEnd;
  }

  sysEmu_->mmioWrite(queueInfo.bufferAddress_ + sizeof(CircBuffCb) + queueInfo.cb_.head_offset, commandSize, command);

  // Update the head offset
  queueInfo.cb_.head_offset = (queueInfo.cb_.head_offset + commandSize) % queueInfo.cb_.length;
  sysEmu_->mmioWrite(queueInfo.bufferAddress_ + offsetof(CircBuffCb, head_offset), sizeof(queueInfo.cb_.head_offset),
                     reinterpret_cast<std::byte*>(&queueInfo.cb_.head_offset));

  // The availability of queue after the command is sent
  if (getAvailSpace(queueInfo.cb_) >= queueInfo.thresholdBytes_) {
    clearEvent = false;
  }

  DV_VLOG(LOW) << "Command sent. Sysemu ptr: " << sysEmu_.get();
  return true;
}

bool DeviceSysEmu::sendCommandMasterMinion(int, int sqIdx, std::byte* command, size_t commandSize, bool,
                                           bool isHighPriority) {
  std::lock_guard lock(mutex_);
  Checker checker{*this};
  auto sq_idx = static_cast<uint32_t>(sqIdx);
  if (sq_idx >= (isHighPriority ? hpSubmissionQueuesMM_.size() : submissionQueuesMM_.size())) {
    throw Exception("Invalid queue");
  }

  bool clearEvent = true;
  auto res = sendCommand(isHighPriority ? hpSubmissionQueuesMM_[sq_idx] : submissionQueuesMM_[sq_idx], command,
                         commandSize, clearEvent);
  if (res) {
    sysEmu_->raiseDevicePuPlicPcieMessageInterrupt();
  }

  // No bitmap for high priority queues
  if (clearEvent && !isHighPriority) {
    // clear corresponding bit
    mmSqBitmap_ &= ~(1U << sq_idx);
  }

  return res;
}

bool DeviceSysEmu::sendCommandServiceProcessor(int, std::byte* command, size_t commandSize) {
  std::lock_guard lock(mutex_);
  Checker checker{*this};
  bool clearEvent = true;
  auto res = sendCommand(submissionQueueSP_, command, commandSize, clearEvent);
  if (res) {
    sysEmu_->raiseDeviceSpioPlicPcieMessageInterrupt();
  }

  if (clearEvent) {
    spSqReady_ = false;
  }

  return res;
}

// EPOLLIN indicates the availability of read event i.e. completion queue
// is available for reads
bool DeviceSysEmu::checkForEventEPOLLIN(const QueueInfo& queueInfo) const {
  Checker checker{*this};
  // Pull the latest state of buffer
  CircBuffCb cb;
  sysEmu_->mmioRead(queueInfo.bufferAddress_, sizeof(cb), reinterpret_cast<std::byte*>(&cb));

  return getUsedSpace(cb) > 0;
}

// EPOLLOUT indicates the availability of write event i.e. submission queue
// is available for writes
bool DeviceSysEmu::checkForEventEPOLLOUT(const QueueInfo& queueInfo) const {
  Checker checker{*this};
  // Pull the latest state of buffer
  CircBuffCb cb;
  sysEmu_->mmioRead(queueInfo.bufferAddress_, sizeof(cb), reinterpret_cast<std::byte*>(&cb));

  return getAvailSpace(cb) >= queueInfo.thresholdBytes_;
}

bool DeviceSysEmu::foundEventsMasterMinion(uint64_t& sqBitmap, bool& cqAvailable) {
  uint64_t tempSqBitmap = 0;
  bool tempCqAvailable = false;
  // Check for SQ availability if it's corresponding interrupt is received
  if (mmIntrptBitmap_ & MM_SQ) {
    // Clear interrupt
    mmIntrptBitmap_ &= ~static_cast<uint32_t>(MM_SQ);
    for (uint32_t sqIdx = 0; sqIdx < mmInfo_.vq_attr.sq_count; ++sqIdx) {
      if (mmSqBitmap_ & 0x1U << sqIdx) {
        continue;
      }
      if (checkForEventEPOLLOUT(submissionQueuesMM_[sqIdx])) {
        tempSqBitmap |= (0x1U << sqIdx);
      }
    }
  }

  // Check for CQ availability if it's corresponding interrupt is received
  if ((mmIntrptBitmap_ & MM_CQ) && !mmCqReady_) {
    // Clear interrupt
    mmIntrptBitmap_ &= ~static_cast<uint32_t>(MM_CQ);
    tempCqAvailable = checkForEventEPOLLIN(completionQueueMM_);
  }
  // return true if edge-trigger event(s) found i.e., some bit from sqBitmap or cqReady activates
  // (changes from  0 -> 1), mimic the PCIe driver
  if ((tempSqBitmap & ~mmSqBitmap_) || (tempCqAvailable && !mmCqReady_)) {
    sqBitmap = tempSqBitmap;
    mmSqBitmap_ = tempSqBitmap;
    cqAvailable = tempCqAvailable;
    mmCqReady_ = tempCqAvailable;
    return true;
  }
  return false;
}

void DeviceSysEmu::waitForEpollEventsMasterMinion(int, uint64_t& sq_bitmap, bool& cq_available,
                                                  std::chrono::milliseconds timeout) {
  DV_VLOG(HIGH) << "Waiting for interrupt from master minion";
  Checker checker{*this};
  sq_bitmap = 0;
  cq_available = false;

  auto lock = std::unique_lock<std::mutex>(mutex_);
  mmEpollBlock_.wait_for(lock, timeout, [this, &sq_bitmap, &cq_available]() {
    if (!isRunning_) {
      return true;
    }
    return foundEventsMasterMinion(sq_bitmap, cq_available);
  });
  DV_VLOG(HIGH) << "Finished waiting interrupt for master minion. SQ_BITMAP: " << std::hex << sq_bitmap
                << " CQ_AVAILABLE: " << cq_available;
}

bool DeviceSysEmu::foundEventsServiceProcessor(bool& sqAvailable, bool& cqAvailable) {
  bool tempSqAvailable = false;
  bool tempCqAvailable = false;
  // Check for SQ availability if it's corresponding interrupt is received
  if ((spIntrptBitmap_ & SP_SQ) && !spSqReady_) {
    // Clear interrupt
    spIntrptBitmap_ &= ~static_cast<uint32_t>(SP_SQ);
    tempSqAvailable = checkForEventEPOLLOUT(submissionQueueSP_);
  }
  // Check for CQ availability if it's corresponding interrupt is received
  if ((spIntrptBitmap_ & SP_CQ) && !spCqReady_) {
    // Clear interrupt
    spIntrptBitmap_ &= ~static_cast<uint32_t>(SP_CQ);
    tempCqAvailable = checkForEventEPOLLIN(completionQueueSP_);
  }
  if ((tempSqAvailable && !spSqReady_) || (tempCqAvailable && !spCqReady_)) {
    sqAvailable = tempSqAvailable;
    spSqReady_ = tempSqAvailable;
    cqAvailable = tempCqAvailable;
    spCqReady_ = tempCqAvailable;
    return true;
  }
  return false;
}

void DeviceSysEmu::waitForEpollEventsServiceProcessor(int, bool& sq_available, bool& cq_available,
                                                      std::chrono::milliseconds timeout) {
  DV_VLOG(HIGH) << "Waiting for interrupt from service processor";
  Checker checker{*this};

  sq_available = false;
  cq_available = false;

  auto lock = std::unique_lock<std::mutex>(mutex_);
  spEpollBlock_.wait_for(lock, timeout, [this, &sq_available, &cq_available]() {
    if (!isRunning_) {
      return true;
    }
    return foundEventsServiceProcessor(sq_available, cq_available);
  });
  DV_VLOG(HIGH) << "Finished waiting interrupt for service processor. SQ_AVAILABLE: " << std::hex << sq_available
                << " CQ_AVAILABLE: " << cq_available;
}

void DeviceSysEmu::setSqThresholdMasterMinion(int, int idx, uint32_t bytesNeeded) {
  std::lock_guard lock(mutex_);
  auto sqIdx = static_cast<uint32_t>(idx);
  if (!bytesNeeded || bytesNeeded > (submissionQueuesMM_[sqIdx].size_ - sizeof(CircBuffCb))) {
    throw Exception("Invalid value for bytesNeeded");
  }
  submissionQueuesMM_[sqIdx].thresholdBytes_ = bytesNeeded;
}

void DeviceSysEmu::setSqThresholdServiceProcessor(int, uint32_t bytesNeeded) {
  std::lock_guard lock(mutex_);
  if (!bytesNeeded || bytesNeeded > (submissionQueueSP_.size_ - sizeof(CircBuffCb))) {
    throw Exception("Invalid value for bytesNeeded");
  }
  submissionQueueSP_.thresholdBytes_ = bytesNeeded;
}

bool DeviceSysEmu::receiveResponse(QueueInfo& queue, std::vector<std::byte>& response, bool& clearEvent) {
  Checker checker{*this};
  clearEvent = true;

  // read queue info
  sysEmu_->mmioRead(queue.bufferAddress_ + offsetof(CircBuffCb, head_offset), sizeof(queue.cb_.head_offset),
                    reinterpret_cast<std::byte*>(&queue.cb_.head_offset));

  // helper function to pop from the circular buffer till size (in bytes) into dst, warping the tail offset if needed
  auto bufferPopWraping = [this, &queue](size_t size, std::byte* dst) {
    // check if there are some messages to read
    if (getUsedSpace(queue.cb_) < size) {
      return 0UL;
    }
    if (queue.cb_.tail_offset + size > queue.cb_.length) {
      auto bytesUntillEnd = queue.cb_.length - queue.cb_.tail_offset;
      sysEmu_->mmioRead(queue.bufferAddress_ + sizeof(CircBuffCb) + queue.cb_.tail_offset, bytesUntillEnd, dst);
      queue.cb_.tail_offset = 0;
      size -= bytesUntillEnd;
      dst += bytesUntillEnd;
    }
    sysEmu_->mmioRead(queue.bufferAddress_ + sizeof(CircBuffCb) + queue.cb_.tail_offset, size, dst);
    queue.cb_.tail_offset = (queue.cb_.tail_offset + size) % queue.cb_.length;
    return size;
  };

  // make room to pop message header
  response.resize(kCommonHeaderSize);
  // pop message header
  if (bufferPopWraping(kCommonHeaderSize, response.data()) <= 0) {
    response.clear();
    return false;
  }

  // get the response size
  uint16_t respSize;
  std::memcpy(&respSize, response.data(), sizeof(respSize));
  if (respSize == 0) {
    throw Exception("CompletionQueue: Invalid response size");
  }
  // make room for message payload
  response.resize(respSize + kCommonHeaderSize);
  // pop the message payload
  if (bufferPopWraping(respSize, &response[kCommonHeaderSize]) <= 0) {
    throw Exception("CompletionQueue: Couldn't read the response payload. ");
  }

  // write the tail offset in circular buffer shared memory
  sysEmu_->mmioWrite(queue.bufferAddress_ + offsetof(CircBuffCb, tail_offset), sizeof(queue.cb_.tail_offset),
                     reinterpret_cast<std::byte*>(&queue.cb_.tail_offset));

  // The availability of queue after the response is received
  if (getUsedSpace(queue.cb_) > 0) {
    clearEvent = false;
  }

  return true;
}

bool DeviceSysEmu::receiveResponseMasterMinion(int, std::vector<std::byte>& response) {
  DV_VLOG(HIGH) << "Start receiving response from Master Minion";
  std::lock_guard lock(mutex_);
  bool clearEvent = true;
  auto tmp = receiveResponse(completionQueueMM_, response, clearEvent);
  if (clearEvent) {
    mmCqReady_ = false;
  }
  DV_VLOG(HIGH) << "Response from Master Minion received: " << (tmp ? "true" : "false");
  return tmp;
}

bool DeviceSysEmu::receiveResponseServiceProcessor(int, std::vector<std::byte>& response) {
  DV_VLOG(HIGH) << "Start receiving response from Service Processor";
  std::lock_guard lock(mutex_);
  bool clearEvent = true;
  auto tmp = receiveResponse(completionQueueSP_, response, clearEvent);
  if (clearEvent) {
    spCqReady_ = false;
  }
  DV_VLOG(HIGH) << "Response from Service Processor received: " << (tmp ? "true" : "false");
  return tmp;
}

void DeviceSysEmu::setupServiceProcessor() {
  auto start = std::chrono::steady_clock::now();
  auto end = start + kVirtualQueuesDiscoveryTimeout;

  while (std::chrono::steady_clock::now() < end) {
    int16_t status;
    sysEmu_->mmioRead(spDevIntfRegAddr_ + offsetof(SP_DEV_INTF_REG, generic_attr) +
                        offsetof(SP_DEV_INTF_GENERIC_ATTR, status),
                      sizeof(status), reinterpret_cast<std::byte*>(&status));
    DV_DLOG(INFO) << "SP DIRs and VQs discovery in progress...\n";
    if (status >= SP_DEV_INTF_SP_BOOT_STATUS_DEV_READY) {
      // SP ready
      sysEmu_->mmioRead(spDevIntfRegAddr_, sizeof(spInfo_), reinterpret_cast<std::byte*>(&spInfo_));
      // calculate crc32 and match it
      boost::crc_32_type crc32Result;
      crc32Result.process_bytes(
        &spInfo_.vq_attr, static_cast<size_t>(spInfo_.generic_attr.total_size - spInfo_.generic_attr.attributes_size));
      DV_DLOG(INFO) << "SP DIRs CRC32 Checksum => Memory: " << spInfo_.generic_attr.crc32
                    << " Calculated: " << crc32Result.checksum();
      if (spInfo_.generic_attr.crc32 != crc32Result.checksum()) {
        throw Exception("SP DIRs CRC32 Checksum failure!");
      }

      auto bar = spInfo_.mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER].bar;
      auto barOffset = spInfo_.mem_regions[SP_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER].bar_offset;
      auto sqOffset = spInfo_.vq_attr.sq_offset;
      auto sqCount = spInfo_.vq_attr.sq_count;
      size_t sqSize = spInfo_.vq_attr.per_sq_size;
      auto cqOffset = spInfo_.vq_attr.cq_offset;
      auto cqCount = spInfo_.vq_attr.cq_count;
      size_t cqSize = spInfo_.vq_attr.per_cq_size;
      DV_DLOG(INFO) << "SP Virtual Queues Descriptor => "
                    << "bar: " << static_cast<uint16_t>(bar) << " barOffset: " << barOffset << " sqOffset: " << sqOffset
                    << " sqCount: " << sqCount << " sqSize: " << sqSize << " cqOffset: " << cqOffset
                    << " cqCount: " << cqCount << " cqSize: " << cqSize;

      assert(bar == 0 || bar == 2);
      // single submission queue model
      submissionQueueSP_.bufferAddress_ = barAddress_[bar] + barOffset + sqOffset;
      submissionQueueSP_.size_ = sqSize * sqCount;
      submissionQueueSP_.thresholdBytes_ = static_cast<uint32_t>(submissionQueueSP_.size_ - sizeof(CircBuffCb)) / 4;
      sysEmu_->mmioRead(submissionQueueSP_.bufferAddress_, sizeof(submissionQueueSP_.cb_),
                        reinterpret_cast<std::byte*>(&submissionQueueSP_.cb_));
      // single completion queue model
      completionQueueSP_.bufferAddress_ = barAddress_[bar] + barOffset + cqOffset;
      completionQueueSP_.size_ = cqSize * cqCount;
      sysEmu_->mmioRead(completionQueueSP_.bufferAddress_, sizeof(completionQueueSP_.cb_),
                        reinterpret_cast<std::byte*>(&completionQueueSP_.cb_));
      DV_DLOG(INFO) << "SP Submission and Completion queue initialized!";
      return;
    } else if (status < 0) {
      throw Exception("SP DIRs and VQs discovery failed!");
    }
    std::this_thread::sleep_for(kPollingInterval);
  }
  throw Exception("Timeout SP Virtual queue discovery");
}

void DeviceSysEmu::setupMasterMinion() {
  auto start = std::chrono::steady_clock::now();
  auto end = start + kVirtualQueuesDiscoveryTimeout;

  while (std::chrono::steady_clock::now() < end) {
    int16_t status;
    sysEmu_->mmioRead(mmDevIntfRegAddr_ + offsetof(MM_DEV_INTF_REG, generic_attr) +
                        offsetof(MM_DEV_INTF_GENERIC_ATTR, status),
                      sizeof(status), reinterpret_cast<std::byte*>(&status));
    DV_DLOG(INFO) << "MM DIRs and VQs discovery in progress...\n";
    if (status >= MM_DEV_INTF_MM_BOOT_STATUS_MM_READY) {
      // MM ready
      sysEmu_->mmioRead(mmDevIntfRegAddr_, sizeof(mmInfo_), reinterpret_cast<std::byte*>(&mmInfo_));
      // calculate crc32 and match it
      boost::crc_32_type crc32Result;
      crc32Result.process_bytes(
        &mmInfo_.vq_attr, static_cast<size_t>(mmInfo_.generic_attr.total_size - mmInfo_.generic_attr.attributes_size));
      if (mmInfo_.generic_attr.crc32 != crc32Result.checksum()) {
        throw Exception("MM DIRs CRC32 Checksum mismatched! (received: " + std::to_string(mmInfo_.generic_attr.crc32) +
                        " vs calculated: " + std::to_string(crc32Result.checksum()) + ")");
      }

      auto bar = mmInfo_.mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER].bar;
      auto barOffset = mmInfo_.mem_regions[MM_DEV_INTF_MEM_REGION_TYPE_VQ_BUFFER].bar_offset;
      auto hpSqOffset = mmInfo_.vq_attr.sq_hp_offset;
      auto hpSqCount = mmInfo_.vq_attr.sq_hp_count;
      size_t hpSqSize = mmInfo_.vq_attr.per_sq_hp_size;
      auto sqOffset = mmInfo_.vq_attr.sq_offset;
      auto sqCount = mmInfo_.vq_attr.sq_count;
      size_t sqSize = mmInfo_.vq_attr.per_sq_size;
      auto cqOffset = mmInfo_.vq_attr.cq_offset;
      auto cqCount = mmInfo_.vq_attr.cq_count;
      size_t cqSize = mmInfo_.vq_attr.per_cq_size;
      DV_DLOG(INFO) << "MM Virtual Queues Descriptor =>"
                    << "\nbar: " << static_cast<uint16_t>(bar) << " barOffset: " << barOffset
                    << "\nhpSqOffset: " << hpSqOffset << " hpSqCount: " << hpSqCount << " hpSqSize: " << hpSqSize
                    << "\nsqOffset: " << sqOffset << " sqCount: " << sqCount << " sqSize: " << sqSize
                    << "\ncqOffset: " << cqOffset << " cqCount: " << cqCount << " cqSize: " << cqSize;
      // init HP SQs
      for (uint8_t hpSqIdx = 0; hpSqIdx < hpSqCount; ++hpSqIdx) {
        QueueInfo hpSqInfo;
        hpSqInfo.bufferAddress_ = barAddress_[bar] + barOffset + hpSqOffset + hpSqIdx * hpSqSize;
        hpSqInfo.size_ = hpSqSize;
        hpSqInfo.thresholdBytes_ = static_cast<uint32_t>(hpSqSize);
        sysEmu_->mmioRead(hpSqInfo.bufferAddress_, sizeof(hpSqInfo.cb_), reinterpret_cast<std::byte*>(&hpSqInfo.cb_));
        hpSubmissionQueuesMM_.emplace_back(hpSqInfo);
      }

      // init SQs
      for (uint8_t i = 0; i < sqCount; ++i) {
        QueueInfo sqInfo;
        sqInfo.bufferAddress_ = barAddress_[bar] + barOffset + sqOffset + i * sqSize;
        sqInfo.size_ = sqSize;
        sqInfo.thresholdBytes_ = kMMThresholdBytes;
        sysEmu_->mmioRead(sqInfo.bufferAddress_, sizeof(sqInfo.cb_), reinterpret_cast<std::byte*>(&sqInfo.cb_));
        submissionQueuesMM_.emplace_back(sqInfo);
      }

      // single completion queue model
      completionQueueMM_.bufferAddress_ = barAddress_[bar] + barOffset + cqOffset;
      completionQueueMM_.size_ = cqSize;
      sysEmu_->mmioRead(completionQueueMM_.bufferAddress_, sizeof(completionQueueMM_.cb_),
                        reinterpret_cast<std::byte*>(&completionQueueMM_.cb_));
      return;
    } else if (status < 0) {
      throw Exception("MM DIRs and VQs discovery failed!");
    }
    std::this_thread::sleep_for(kPollingInterval);
  }
  throw Exception("Timeout MM virtual queue discovery");
}

void* DeviceSysEmu::allocDmaBuffer(int, size_t sizeInBytes, bool) {
  if (sizeInBytes > getFreeCmaMemory()) {
    throw Exception("Not enough CMA memory");
  }
  auto res = malloc(sizeInBytes);
  if (!res) {
    throw Exception("Error allocating memory buffer");
  }
  return res;
}

void DeviceSysEmu::freeDmaBuffer(void* dmaBuffer) {
  if (!dmaBuffer) {
    throw Exception("Can't free a null pointer");
  }
  free(dmaBuffer);
}

DeviceConfig DeviceSysEmu::getDeviceConfig(int) {
  return DeviceConfig{
    DeviceConfig::FormFactor::PCIE,                                 /* form factor */
    25,                                                             /* tdp (W) */
    32,                                                             /* L3 cache (MB) */
    16,                                                             /* L2 cache (MB) */
    80,                                                             /* scratchpad (MB) */
    64,                                                             /* cache line size (bytes) */
    4,                                                              /* L2 cache banks */
    128000,                                                         /* DDR BW (MB/s) */
    1000,                                                           /* Mhz */
    static_cast<uint32_t>(spInfo_.generic_attr.minion_shires_mask), /* shire mask */
    32,                                                             /* spare minion shire id */
    0                                                               /* Arch revision 0 (ETSOC1) */
  };
}

int DeviceSysEmu::getActiveShiresNum(int device) {
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

uint32_t DeviceSysEmu::getFrequencyMHz(int device) {
  DeviceConfig config = getDeviceConfig(device);
  return config.minionBootFrequency_;
}

size_t DeviceSysEmu::getFreeCmaMemory() const {
  return 1ULL << 30; // default free memory 1 GiB
}

void DeviceSysEmu::checkSysemuLastError() const {
  auto lastError = hostListener_.getSysemuLastError();
  if (lastError.has_value()) {
    throw Exception(lastError.value());
  }
}
