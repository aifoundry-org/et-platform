/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
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

