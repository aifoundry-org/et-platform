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

DeviceSysEmuMulti::DeviceSysEmuMulti(std::vector<emu::SysEmuOptions> options) {
  for (auto& o : options) {
    devices_.emplace_back(std::make_unique<DeviceSysEmu>(o));
  }
}

bool DeviceSysEmuMulti::sendCommandMasterMinion(int device, int sqIdx, std::byte* command, size_t commandSize,
                                                CmdFlagMM flags) {
  return getDevice(device).sendCommandMasterMinion(device, sqIdx, command, commandSize, flags);
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

bool DeviceSysEmuMulti::sendCommandServiceProcessor(int device, std::byte* command, size_t commandSize,
                                                    CmdFlagSP flags) {
  return getDevice(device).sendCommandServiceProcessor(device, command, commandSize, flags);
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
uint64_t DeviceSysEmuMulti::getDramSize(int device) const {
  return getDevice(device).getDramSize(device);
}
uint64_t DeviceSysEmuMulti::getDramBaseAddress(int device) const {
  return getDevice(device).getDramBaseAddress(device);
}
void* DeviceSysEmuMulti::allocDmaBuffer(int device, size_t sizeInBytes, bool writeable) {
  return getDevice(device).allocDmaBuffer(device, sizeInBytes, writeable);
}
void DeviceSysEmuMulti::freeDmaBuffer(void* dmaBuffer) {
  return getDevice(0).freeDmaBuffer(dmaBuffer);
}
size_t DeviceSysEmuMulti::getTraceBufferSizeMasterMinion(int device, TraceBufferType traceType) {
  return getDevice(device).getTraceBufferSizeMasterMinion(device, traceType);
}
bool DeviceSysEmuMulti::getTraceBufferServiceProcessor(int device, TraceBufferType traceType,
                                                       std::vector<std::byte>& traceBuf) {
  return getDevice(device).getTraceBufferServiceProcessor(device, traceType, traceBuf);
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
std::string DeviceSysEmuMulti::getDeviceAttribute(int device, std::string relAttrPath) const {
  return getDevice(device).getDeviceAttribute(device, relAttrPath);
}
void DeviceSysEmuMulti::clearDeviceAttributes(int device, std::string relGroupPath) const {
  return getDevice(device).clearDeviceAttributes(device, relGroupPath);
}
void DeviceSysEmuMulti::reinitDeviceInstance(int device, bool masterMinionOnly, std::chrono::milliseconds timeout) {
  getDevice(device).reinitDeviceInstance(device, masterMinionOnly, timeout);
}
void DeviceSysEmuMulti::hintInactivity(int device) {
  return getDevice(device).hintInactivity(device);
}
