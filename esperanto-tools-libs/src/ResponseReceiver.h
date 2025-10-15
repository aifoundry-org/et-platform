/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/
#pragma once

#include "EventManager.h"
#include "ExecutionContextCache.h"
#include "device-layer/IDeviceLayer.h"

#include <atomic>
#include <functional>
#include <memory>
#include <thread>

namespace rt {
class ResponseReceiver {
public:
  struct IReceiverServices {
    virtual ~IReceiverServices() = default;
    virtual bool areEventsOnFly(DeviceId device) const = 0;
    virtual void checkDevice(DeviceId device) = 0;
    virtual void onResponseReceived(DeviceId device, const std::vector<std::byte>& response) = 0;
  };
  explicit ResponseReceiver(dev::IDeviceLayer& deviceLayer, IReceiverServices* receiverServices);

  void startDeviceChecker();

  ~ResponseReceiver();

private:
  void checkResponses(int deviceId);
  void checkDevices();

  std::vector<std::thread> receivers_;
  std::thread deviceChecker_;
  bool runDeviceChecker_ = false;
  bool runReceiver_ = true;
  dev::IDeviceLayer& deviceLayer_;
  IReceiverServices* receiverServices_;
};
} // namespace rt
