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
#include "EventManager.h"
#include "ExecutionContextCache.h"
#include "device-layer/IDeviceLayer.h"
#include <atomic>
#include <functional>
#include <thread>

namespace rt {
class ResponseReceiver {
public:
  struct IReceiverServices {
    virtual ~IReceiverServices() = default;
    virtual void getDevicesWithEventsOnFly(std::vector<int>& outResult) const = 0;
    virtual void checkDevice(DeviceId device) = 0;
    virtual void onResponseReceived(DeviceId device, const std::vector<std::byte>& response) = 0;
  };
  explicit ResponseReceiver(dev::IDeviceLayer* deviceLayer, IReceiverServices* receiverServices);

  void startDeviceChecker();

  ~ResponseReceiver();

private:
  void checkResponses();
  void checkDevices();

  std::thread receiver_;
  std::thread deviceChecker_;
  bool runDeviceChecker_ = false;
  bool runReceiver_ = true;
  dev::IDeviceLayer* deviceLayer_;
  IReceiverServices* receiverServices_;
};
} // namespace rt
