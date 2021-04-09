/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#pragma once
#include "KernelParametersCache.h"
#include "EventManager.h"
#include "device-layer/IDeviceLayer.h"
#include <atomic>
#include <functional>
#include <thread>

namespace rt {
class ResponseReceiver {
public:
  using Callback = std::function<void(const std::vector<std::byte>& response)>;  
  explicit ResponseReceiver(dev::IDeviceLayer* deviceLayer, Callback responseCallback);
  ~ResponseReceiver();

private:
  std::thread receiver_;
  std::atomic<bool> run_;
  dev::IDeviceLayer* deviceLayer_;
  Callback responseCallback_;
};
} // namespace rt
