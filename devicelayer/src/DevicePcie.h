/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#pragma once
#include "deviceLayer/IDeviceLayer.h"
#include <et_ioctl.h>
#include <mutex>
#include <unordered_map>

namespace dev {

class DevicePcie : public IDeviceLayer {
public:
  explicit DevicePcie(bool enableOps = true, bool enableMngmt = true);
  ~DevicePcie();

  // IDeviceAsync
  bool sendCommandMasterMinion(int device, int sqIdx, std::byte* command, size_t commandSize, bool isDma,
                               bool isHighPriority) override;
  void setSqThresholdMasterMinion(int device, int sqIdx, uint32_t bytesNeeded) override;
  void waitForEpollEventsMasterMinion(int device, uint64_t& sq_bitmap, bool& cq_available,
                                      std::chrono::milliseconds timeout = std::chrono::seconds(10)) override;
  bool receiveResponseMasterMinion(int device, std::vector<std::byte>& response) override;

  bool sendCommandServiceProcessor(int device, std::byte* command, size_t commandSize) override;
  void setSqThresholdServiceProcessor(int device, uint32_t bytesNeeded) override;
  void waitForEpollEventsServiceProcessor(int device, bool& sq_available, bool& cq_available,
                                          std::chrono::milliseconds timeout = std::chrono::seconds(10)) override;
  bool receiveResponseServiceProcessor(int device, std::vector<std::byte>& response) override;

  // IDeviceSync
  DmaInfo getDmaInfo(int device) const override;
  int getDevicesCount() const override;
  int getSubmissionQueuesCount(int device) const override;
  DeviceState getDeviceStateMasterMinion(int device) const override;
  DeviceState getDeviceStateServiceProcessor(int device) const override;
  size_t getSubmissionQueueSizeMasterMinion(int device) const override;
  size_t getSubmissionQueueSizeServiceProcessor(int device) const override;
  bool getTraceBufferServiceProcessor(int device, TraceBufferType trace_type,
                                      std::vector<std::byte>& response) override;
  size_t getTraceBufferSizeMasterMinion(int device, TraceBufferType traceType) override;
  int updateFirmwareImage(int device, std::vector<unsigned char>& fwImage) override;
  int getDmaAlignment() const override;
  uint64_t getDramSize() const override;
  uint64_t getDramBaseAddress() const override;
  void* allocDmaBuffer(int device, size_t sizeInBytes, bool writeable) override;
  void freeDmaBuffer(void* dmaBuffer) override;
  DeviceConfig getDeviceConfig(int device) override;
  int getActiveShiresNum(int device) override;
  uint32_t getFrequencyMHz(int device) override;
  size_t getFreeCmaMemory() const override;

private:
  struct DevInfo {
    dram_info userDram_;
    DeviceConfig cfg_;
    uint16_t mmSqCount_;
    uint16_t spSqMaxMsgSize_;
    uint16_t mmSqMaxMsgSize_;
    int fdOps_;
    int epFdOps_;
    int fdMgmt_;
    int epFdMgmt_;
  };

  std::unordered_map<void*, size_t> dmaBuffers_;
  std::vector<DevInfo> devices_;
  // this mutex is only needed to keep the map of dmaBuffers, if at some point we decide we don't need them, we can
  // remove the mutex
  std::mutex mutex_;
  bool opsEnabled_;
  bool mngmtEnabled_;
};
} // namespace dev
