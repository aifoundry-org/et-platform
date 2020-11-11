/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "TargetSysEmu.h"
#include "RPCDevice/SysEmuLauncher.h"
#include "RPCDevice/TargetSysEmu.h"
#include <vector>
using namespace rt;

//#see SW-4433. There are a lot of tech debt in current implementation.

TargetSysEmu::TargetSysEmu() {
  device_ = std::make_unique<et_runtime::device::TargetSysEmu>(0);
  device_->init(); // we shouldn't have to call these functions explicitly
  firmware_.loadOnDevice(device_.get());
  firmware_.configureFirmware(device_.get());
  firmware_.bootFirmware(device_.get());
  device_->postFWLoadInit();
}

size_t TargetSysEmu::getDramSize() const { return device_->dramSize(); }
uint64_t TargetSysEmu::getDramBaseAddr() const { return device_->dramBaseAddr(); }

std::vector<DeviceId> TargetSysEmu::getDevices() const {
  static DeviceId d;
  // TODO. Whenever we support multiple devices, fix this. #SW-4438
  return {d};
}

bool TargetSysEmu::writeMailbox(const void* src, size_t size) {
  return device_->mb_write(src, size);
}

bool TargetSysEmu::readMailbox(std::byte* dst, size_t size, std::chrono::milliseconds blockingPeriod) {
  return device_->mb_read(dst, size, blockingPeriod) > 0;
}

bool TargetSysEmu::writeDevMemDMA(uintptr_t dev_addr, size_t size, const void* buf) {
  return device_->writeDevMemDMA(dev_addr, size, buf);
}

bool TargetSysEmu::readDevMemDMA(uintptr_t dev_addr, size_t size, void* buf) {
  return device_->readDevMemDMA(dev_addr, size, buf);
}

bool TargetSysEmu::writeDevMemMMIO(uintptr_t dev_addr, size_t size, const void* buf) {
  return device_->writeDevMemMMIO(dev_addr, size, buf);
}

TargetSysEmu::~TargetSysEmu() {
  device_->deinit(); // TODO. see SW-4433; we shouldn't call anything like this explicetly
}