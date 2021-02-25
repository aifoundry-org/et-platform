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
#include "esperanto/device-api/device_api_message_types.h"
#include "esperanto/device-api/device_api_spec_non_privileged.h"
#include "utils.h"
#include <algorithm>
#include <array>
#include <thread>
using namespace rt;

namespace {
using namespace std::chrono_literals;
auto kPollingInterval = 100ms;
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
      if (devicesToCheck.empty()) {
        std::this_thread::sleep_for(kPollingInterval);
      } else {
        std::random_shuffle(begin(devicesToCheck), end(devicesToCheck));
        int responsesCount = 0;
        for (auto dev : devicesToCheck) {
          if (deviceLayer_->receiveResponseMasterMinion(dev, buffer)) {
            RT_LOG(INFO) << "Got response from deviceId: " << dev;
            responsesCount++;
            receiverServices_->onResponseReceived(buffer);
          }
        }

        if (responsesCount == 0) {
          for (auto dev : devicesToCheck) {
            uint64_t sq_bitmap;
            bool cq_available;
            deviceLayer_->waitForEpollEventsMasterMinion(dev, sq_bitmap, cq_available);
          }
        }
      }
    }
  });
}

ResponseReceiver::~ResponseReceiver() {
  run_ = false;
  receiver_.join();
}
