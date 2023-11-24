/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "DevicePcie.h"
#include "DeviceSysEmuMulti.h"
#include <sw-sysemu/SysEmuOptions.h>

using namespace dev;

std::unique_ptr<IDeviceLayer> IDeviceLayer::createSysEmuDeviceLayer(const emu::SysEmuOptions& options,
                                                                    uint8_t numDevices) {
  auto v = std::vector<emu::SysEmuOptions>();
  for (int i = 0; i < numDevices; ++i) {
    v.emplace_back(options);
  }
  return std::make_unique<DeviceSysEmuMulti>(v);
}

std::unique_ptr<IDeviceLayer> IDeviceLayer::createSysEmuDeviceLayer(std::vector<emu::SysEmuOptions> options) {
  return std::make_unique<DeviceSysEmuMulti>(options);
}

std::unique_ptr<IDeviceLayer> IDeviceLayer::createPcieDeviceLayer(bool enableMasterMinion,
                                                                  bool enableServiceProcessor) {
  return std::make_unique<DevicePcie>(enableMasterMinion, enableServiceProcessor);
}

