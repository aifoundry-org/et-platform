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
#include "utils.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <thread>
#include <utility>
using namespace rt;

namespace {
using namespace std::chrono_literals;
auto kPollingInterval = 10ms;
} // namespace

ResponseReceiver::ResponseReceiver(dev::IDeviceLayer* deviceLayer, IReceiverServices* receiverServices)
  : run_(true)
  , deviceLayer_(deviceLayer)
  , receiverServices_(receiverServices) {

  receiver_ = std::thread([this]() {
    // Max ioctl size is 14b
    constexpr uint32_t kMaxMsgSize = (1ul << 14) - 1;

    std::vector<std::byte> buffer(kMaxMsgSize);
    
    while (run_) {
      auto devicesToCheck = receiverServices_->getDevicesWithEventsOnFly();
      std::random_shuffle(begin(devicesToCheck), end(devicesToCheck));
      int responsesCount = 0;
      for (auto dev : devicesToCheck) {
        while (deviceLayer_->receiveResponseMasterMinion(dev, buffer)) {
          RT_VLOG(HIGH) << "Got response from deviceId: " << dev;
          responsesCount++;
          receiverServices_->onResponseReceived(buffer);
          RT_VLOG(HIGH) << "Response processed";
        }
      }
      if (responsesCount == 0) {
        std::this_thread::sleep_for(kPollingInterval);
      }
    }
  });
}

ResponseReceiver::~ResponseReceiver() {
  RT_LOG(INFO) << "Destroying response receiver";
  run_ = false;
  receiver_.join();
  RT_LOG(INFO) << "Response receiver destroyed";
}
