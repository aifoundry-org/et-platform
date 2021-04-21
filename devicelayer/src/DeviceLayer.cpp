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
#include "DevicePcie.h"

using namespace dev;

std::unique_ptr<IDeviceLayer> IDeviceLayer::createSysEmuDeviceLayer(const emu::SysEmuOptions& options) {
  return std::make_unique<DeviceSysEmu>(options);
}

std::unique_ptr<IDeviceLayer> IDeviceLayer::createPcieDeviceLayer() {
  return std::make_unique<DevicePcie>();
}
