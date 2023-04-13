/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "ResponseReceiver.h"
#include "RuntimeImp.h"
#include "Utils.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <easy/profiler.h>
#include <random>
#include <thread>
#include <utility>

using namespace rt;
using namespace std::chrono_literals;
namespace {
constexpr auto kResponsePollingIntervalWithEventsOnFly = 50us;
constexpr auto kResponsePollingIntervalNoEventsOnFly = 500us;
constexpr auto kResponseNumTriesBeforePolling = 1;
constexpr auto kCheckDevicesInterval = 5s;
constexpr auto kCheckDevicesPolling = 1ms;
} // namespace

void ResponseReceiver::checkResponses(int deviceId) {
  EASY_THREAD_SCOPE("ResponseReceiver")
  // Max ioctl size is 14b
  constexpr uint32_t kMaxMsgSize = (1UL << 14) - 1;

  std::vector<std::byte> buffer(kMaxMsgSize);

  std::random_device rd;
  std::mt19937 gen(rd());

  while (runReceiver_) {
    int responsesCount = 0;
    for (int i = 0; i < kResponseNumTriesBeforePolling; ++i) {
      try {
        while (deviceLayer_->receiveResponseMasterMinion(deviceId, buffer)) {
          RT_VLOG(LOW) << "Got response from deviceId: " << deviceId;
          responsesCount++;
          receiverServices_->onResponseReceived(DeviceId{deviceId}, buffer);
          RT_VLOG(LOW) << "Response processed";
        }
      } catch (const std::exception& e) {
        RT_LOG(WARNING)
          << "Exception in device receiver runner thread. DeviceLayer could be in a BAD STATE. Exception message: "
          << e.what();
      }
    }
    if (responsesCount == 0) {
      // check if there are events on fly
      auto eventsOnfly = receiverServices_->areEventsOnFly(DeviceId{deviceId});
      if (!eventsOnfly) {
        deviceLayer_->hintInactivity(deviceId);
      }
      std::this_thread::sleep_for(eventsOnfly ? kResponsePollingIntervalWithEventsOnFly
                                              : kResponsePollingIntervalNoEventsOnFly);
    }
  }
}

void ResponseReceiver::checkDevices() {
  auto devices = deviceLayer_->getDevicesCount();
  auto lastCheck = std::chrono::high_resolution_clock::now() - kCheckDevicesInterval;
  while (runDeviceChecker_) {
    try {
      if (auto currentTime = std::chrono::high_resolution_clock::now();
          currentTime > (lastCheck + kCheckDevicesInterval)) {
        for (int i = 0; i < devices; ++i) {
          receiverServices_->checkDevice(DeviceId{i});
        }
        lastCheck = currentTime;
      }
    } catch (const std::exception& e) {
      RT_LOG(WARNING)
        << "Exception in device checker runner thread. DeviceLayer could be in a BAD STATE. Exception message: "
        << e.what();
    }
    std::this_thread::sleep_for(kCheckDevicesPolling);
  }
}

ResponseReceiver::ResponseReceiver(dev::IDeviceLayer* deviceLayer, IReceiverServices* receiverServices)
  : deviceLayer_(deviceLayer)
  , receiverServices_(receiverServices) {

  auto devCount = deviceLayer_->getDevicesCount();
  for (int i = 0; i < devCount; ++i) {
    receivers_.emplace_back(std::thread(std::bind(&ResponseReceiver::checkResponses, this, i)));
  }
}

void ResponseReceiver::startDeviceChecker() {
  runDeviceChecker_ = true;
  deviceChecker_ = std::thread(std::bind(&ResponseReceiver::checkDevices, this));
}

ResponseReceiver::~ResponseReceiver() {
  RT_LOG(INFO) << "Destroying response receiver";
  if (runDeviceChecker_) {
    runDeviceChecker_ = false;
    deviceChecker_.join();
  }
  runReceiver_ = false;
  for (auto& t : receivers_) {
    t.join();
  }
  RT_LOG(INFO) << "Response receiver destroyed";
}
