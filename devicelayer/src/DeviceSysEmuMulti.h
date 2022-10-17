/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#pragma once
#include "DeviceSysEmu.h"
#include <chrono>

namespace dev {
class DeviceSysEmuMulti : public IDeviceLayer {
public:
  explicit DeviceSysEmuMulti(std::vector<emu::SysEmuOptions> options);

  // IDeviceAsync
  bool sendCommandMasterMinion(int device, int sqIdx, std::byte* command, size_t commandSize, CmdFlagMM flags) override;
  void setSqThresholdMasterMinion(int device, int sqIdx, uint32_t bytesNeeded) override;
  void waitForEpollEventsMasterMinion(int device, uint64_t& sqBitmap, bool& cqAvailable,
                                      std::chrono::milliseconds timeout = std::chrono::seconds(10)) override;
  bool receiveResponseMasterMinion(int device, std::vector<std::byte>& response) override;

  bool sendCommandServiceProcessor(int device, std::byte* command, size_t commandSize, CmdFlagSP flags) override;
  void setSqThresholdServiceProcessor(int device, uint32_t bytesNeeded) override;
  void waitForEpollEventsServiceProcessor(int device, bool& sqAvailable, bool& cqAvailable,
                                          std::chrono::milliseconds timeout = std::chrono::seconds(10)) override;
  bool receiveResponseServiceProcessor(int device, std::vector<std::byte>& response) override;

  // IDeviceSync
  int getDevicesCount() const override;
  int getSubmissionQueuesCount(int device) const override;
  DeviceState getDeviceStateMasterMinion(int device) const override;
  DeviceState getDeviceStateServiceProcessor(int device) const override;
  size_t getSubmissionQueueSizeMasterMinion(int device) const override;
  size_t getSubmissionQueueSizeServiceProcessor(int device) const override;
  int getDmaAlignment() const override;
  uint64_t getDramSize() const override;
  uint64_t getDramBaseAddress() const override;
  void* allocDmaBuffer(int device, size_t sizeInBytes, bool writeable) override;
  void freeDmaBuffer(void* dmaBuffer) override;
  size_t getTraceBufferSizeMasterMinion(int device, TraceBufferType traceType) override;
  bool getTraceBufferServiceProcessor(int device, TraceBufferType traceType, std::vector<std::byte>& traceBuf) override;
  DeviceConfig getDeviceConfig(int device) override;
  int getActiveShiresNum(int device) override;
  uint32_t getFrequencyMHz(int device) override;
  int updateFirmwareImage(int device, std::vector<unsigned char>& fwImage) override;
  size_t getFreeCmaMemory() const override;
  DmaInfo getDmaInfo(int device) const override;
  std::string getDeviceAttribute(int device, std::string relAttrPath) const override;
  void clearDeviceAttributes(int device, std::string relGroupPath) const override;
  void reinitDeviceInstance(int device, bool masterMinionOnly, std::chrono::milliseconds timeout) override;

private:
  DeviceSysEmu& getDevice(int device);
  const DeviceSysEmu& getDevice(int device) const;
  std::vector<std::unique_ptr<DeviceSysEmu>> devices_;
};
} // namespace dev
