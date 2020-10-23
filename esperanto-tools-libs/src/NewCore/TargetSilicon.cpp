/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "TargetSilicon.h"
#include "PCIEDevice/PCIeDevice.h"

using namespace rt;

//#see SW-4433. There are a lot of tech debt in current implementation.

TargetSilicon::TargetSilicon() {
  device_ = std::make_unique<et_runtime::device::PCIeDevice>(0);
  device_->init();
}

std::vector<DeviceId> TargetSilicon::getDevices() const {
  static DeviceId d;
  // TODO. Whenever we support multiple devices, fix this. #SW-4438
  return {d};
}

bool TargetSilicon::writeDevMemDMA(uintptr_t dev_addr, size_t size, const void* buf) {
  return device_->writeDevMemDMA(dev_addr, size, buf);
}

bool TargetSilicon::readDevMemDMA(uintptr_t dev_addr, size_t size, void* buf) {
  return device_->readDevMemDMA(dev_addr, size, buf);
}

size_t TargetSilicon::getDramSize() const {
  return device_->dramSize();
}
uint64_t TargetSilicon::getDramBaseAddr() const { return device_->dramBaseAddr(); }

TargetSilicon::~TargetSilicon() { device_->deinit(); }