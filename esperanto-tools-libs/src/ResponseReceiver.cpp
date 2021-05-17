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
          while (deviceLayer_->receiveResponseMasterMinion(dev, buffer)) {
            RT_VLOG(HIGH) << "Got response from deviceId: " << dev;
            responsesCount++;
            receiverServices_->onResponseReceived(buffer);
            RT_VLOG(HIGH) << "Response processed";
          }
        }

        if (responsesCount == 0) {
          for (auto dev : devicesToCheck) {
            uint64_t sq_bitmap;
            bool cq_available;
            // https://esperantotech.atlassian.net/browse/SW-7822
            // we are losing somehow interrupts in sysemu, so instead of waitForEpoll lets do a quick & dirty polling
            // worarkound ...
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            // RT_LOG(INFO) << "No responses, waiting for epoll";
            // deviceLayer_->waitForEpollEventsMasterMinion(dev, sq_bitmap, cq_available);
            // RT_LOG(INFO) << "Finished waiting for epoll";
          }
        }
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
