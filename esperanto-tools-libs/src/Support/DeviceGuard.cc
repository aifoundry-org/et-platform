//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/Support/DeviceGuard.h"

#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Core/DeviceManager.h"

#include <cassert>

using namespace et_runtime;

GetDev::GetDev() : dev(getEtDevice()) {
  assert(dev->deviceAlive());
}

GetDev::~GetDev() {  }

std::shared_ptr<et_runtime::Device> GetDev::getEtDevice() {
  auto device_manager = getDeviceManager();
  assert(device_manager);
  auto deviceRet = device_manager->getActiveDevice();
  assert(deviceRet);
  return deviceRet.get();
}
