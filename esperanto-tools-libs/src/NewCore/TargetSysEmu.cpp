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

TargetSysEmu::~TargetSysEmu() {
  device_->deinit(); // TODO. see SW-4433; we shouldn't call anything like this explicetly
}