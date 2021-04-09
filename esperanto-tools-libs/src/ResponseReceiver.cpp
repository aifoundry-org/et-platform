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
#include <thread>
using namespace rt;

ResponseReceiver::ResponseReceiver(dev::IDeviceLayer* deviceLayer, Callback responseCallback)
  : run_(true)
  , deviceLayer_(deviceLayer)
  , responseCallback_(responseCallback) {

  receiver_ = std::thread([this]() {
    // Max ioctl size is 14b
    constexpr uint32_t kMaxMsgSize = (1ul << 14) - 1;

    std::vector<std::byte> buffer(kMaxMsgSize);

    auto devicesToCheck = std::vector<int>(deviceLayer_->getDevicesCount());
    while (run_) {
      std::random_shuffle(begin(devicesToCheck), end(devicesToCheck));
      int responsesCount = 0;
      for (auto dev : devicesToCheck) {
        if (deviceLayer_->receiveResponseMasterMinion(dev, buffer)) {
          RT_LOG(INFO) << "Got response from deviceId: " << dev;
          responsesCount++;
          responseCallback_(buffer);
          RT_LOG(INFO) << "Response processed";
        }
      }
      if (responsesCount == 0) {
        for (auto dev : devicesToCheck) {
          uint64_t sq_bitmap;
          bool cq_available;
          RT_DLOG(INFO) << "No responses, waiting for epoll";
          deviceLayer_->waitForEpollEventsMasterMinion(dev, sq_bitmap, cq_available);
          RT_DLOG(INFO) << "Finished waiting for epoll. SQ_BITMAP: " << std::hex << sq_bitmap  << "CQ_AVAILABLE: " << (cq_available ? "Yes" : "No");
        }
      }
    }
  });
}

ResponseReceiver::~ResponseReceiver() {
  run_ = false;
  receiver_.join();
}
