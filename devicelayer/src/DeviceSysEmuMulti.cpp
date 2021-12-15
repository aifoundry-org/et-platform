/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "DeviceSysEmuMulti.h"
#include "DeviceSysEmu.h"
#include <chrono>

using namespace dev;

DeviceSysEmu& DeviceSysEmuMulti::getDevice(int device) {
  auto dev = static_cast<size_t>(device);
  if (dev >= devices_.size()) {
    throw Exception("Invalid device.");
  }
  return *devices_[dev];
}

const DeviceSysEmu& DeviceSysEmuMulti::getDevice(int device) const {
  auto dev = static_cast<size_t>(device);
  if (dev >= devices_.size()) {
    throw Exception("Invalid device.");
  }
  return *devices_[dev];
}

DmaInfo DeviceSysEmuMulti::getDmaInfo(int device) const {
  return getDevice(device).getDmaInfo(device);
}

DeviceSysEmuMulti::DeviceSysEmuMulti(const emu::SysEmuOptions& options, int numDevices) {
  for (int i = 0; i < numDevices; ++i) {
    devices_.emplace_back(std::make_unique<DeviceSysEmu>(options));
  }
}

bool DeviceSysEmuMulti::sendCommandMasterMinion(int device, int sqIdx, std::byte* command, size_t commandSize,
                                                bool isDma, bool isHighPriority) {
  return getDevice(device).sendCommandMasterMinion(device, sqIdx, command, commandSize, isDma, isHighPriority);
}

void DeviceSysEmuMulti::setSqThresholdMasterMinion(int device, int sqIdx, uint32_t bytesNeeded) {
  return getDevice(device).setSqThresholdMasterMinion(device, sqIdx, bytesNeeded);
}
void DeviceSysEmuMulti::waitForEpollEventsMasterMinion(int device, uint64_t& sqBitmap, bool& cqAvailable,
                                                       std::chrono::milliseconds timeout) {
  return getDevice(device).waitForEpollEventsMasterMinion(device, sqBitmap, cqAvailable, timeout);
}
bool DeviceSysEmuMulti::receiveResponseMasterMinion(int device, std::vector<std::byte>& response) {
  return getDevice(device).receiveResponseMasterMinion(device, response);
}

bool DeviceSysEmuMulti::sendCommandServiceProcessor(int device, std::byte* command, size_t commandSize) {
  return getDevice(device).sendCommandServiceProcessor(device, command, commandSize);
}
void DeviceSysEmuMulti::setSqThresholdServiceProcessor(int device, uint32_t bytesNeeded) {
  return getDevice(device).setSqThresholdServiceProcessor(device, bytesNeeded);
}
void DeviceSysEmuMulti::waitForEpollEventsServiceProcessor(int device, bool& sqAvailable, bool& cqAvailable,
                                                           std::chrono::milliseconds timeout) {
  return getDevice(device).waitForEpollEventsServiceProcessor(device, sqAvailable, cqAvailable, timeout);
}
bool DeviceSysEmuMulti::receiveResponseServiceProcessor(int device, std::vector<std::byte>& response) {
  return getDevice(device).receiveResponseServiceProcessor(device, response);
}

// IDeviceSync
int DeviceSysEmuMulti::getDevicesCount() const {
  return static_cast<int>(devices_.size());
}

int DeviceSysEmuMulti::getSubmissionQueuesCount(int device) const {
  return getDevice(device).getSubmissionQueuesCount(device);
}

DeviceState DeviceSysEmuMulti::getDeviceStateMasterMinion(int device) const {
  return getDevice(device).getDeviceStateMasterMinion(device);
}
DeviceState DeviceSysEmuMulti::getDeviceStateServiceProcessor(int device) const {
  return getDevice(device).getDeviceStateServiceProcessor(device);
}
size_t DeviceSysEmuMulti::getSubmissionQueueSizeMasterMinion(int device) const {
  return getDevice(device).getSubmissionQueueSizeMasterMinion(device);
}
size_t DeviceSysEmuMulti::getSubmissionQueueSizeServiceProcessor(int device) const {
  return getDevice(device).getSubmissionQueueSizeServiceProcessor(device);
}
int DeviceSysEmuMulti::getDmaAlignment() const {
  return getDevice(0).getDmaAlignment();
}
uint64_t DeviceSysEmuMulti::getDramSize() const {
  return getDevice(0).getDramSize();
}
uint64_t DeviceSysEmuMulti::getDramBaseAddress() const {
  return getDevice(0).getDramBaseAddress();
}
void* DeviceSysEmuMulti::allocDmaBuffer(int device, size_t sizeInBytes, bool writeable) {
  return getDevice(device).allocDmaBuffer(device, sizeInBytes, writeable);
}
void DeviceSysEmuMulti::freeDmaBuffer(void* dmaBuffer) {
  return getDevice(0).freeDmaBuffer(dmaBuffer);
}
bool DeviceSysEmuMulti::getTraceBufferServiceProcessor(int device, TraceBufferType trace_type,
                                                       std::vector<std::byte>& response) {
  return getDevice(device).getTraceBufferServiceProcessor(device, trace_type, response);
}
DeviceConfig DeviceSysEmuMulti::getDeviceConfig(int device) {
  return getDevice(device).getDeviceConfig(device);
}
int DeviceSysEmuMulti::getActiveShiresNum(int device) {
  return getDevice(device).getActiveShiresNum(device);
}
uint32_t DeviceSysEmuMulti::getFrequencyMHz(int device) {
  return getDevice(device).getFrequencyMHz(device);
}
int DeviceSysEmuMulti::updateFirmwareImage(int device, std::vector<unsigned char>& fwImage) {
  return getDevice(device).updateFirmwareImage(device, fwImage);
}
size_t DeviceSysEmuMulti::getFreeCmaMemory() const {
  return getDevice(0).getFreeCmaMemory();
}
