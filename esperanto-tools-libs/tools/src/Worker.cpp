/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "Worker.h"
#include "Logging.h"
#include <hostUtils/logging/Logging.h>
using namespace rt;

Worker::Worker(size_t bytesH2D, size_t bytesD2H, DeviceId device, IRuntime& runtime)
  : runtime_(runtime)
  , device_(device) {
  auto devices = runtime.getDevices();
  BM_LOG_IF(FATAL, std::find(begin(devices), end(devices), device_) == end(devices)) << "Invalid DeviceId";
  BM_LOG_IF(FATAL, bytesH2D == 0 && bytesD2H == 0) << "H2D and D2H can't be both zero";
  if (bytesH2D > 0) {
    hH2D_.resize(bytesH2D);
    dH2D_ = runtime_.mallocDevice(device_, bytesH2D);
  }
  if (bytesD2H > 0) {
    hD2H_.resize(bytesD2H);
    dD2H_ = runtime_.mallocDevice(device_, bytesD2H);
  }
  stream_ = runtime_.createStream(device_);
  result_.device = device_;
}

void Worker::start(int numIterations) {
  runner_ = std::thread([this, numIterations] {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numIterations; ++i) {
      if (dH2D_) {
        runtime_.memcpyHostToDevice(stream_, hH2D_.data(), dH2D_, hH2D_.size());
      }
      if (dD2H_) {
        runtime_.memcpyDeviceToHost(stream_, dD2H_, hD2H_.data(), hD2H_.size());
      }
    }
    runtime_.waitForStream(stream_);
    auto et = std::chrono::high_resolution_clock::now() - start;
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(et);
    auto secs = us.count() / 1e6f;
    result_.bytesReceivedPerSecond = hD2H_.size() * numIterations / secs;
    result_.bytesSentPerSecond = hH2D_.size() * numIterations / secs;
  });
}

rt::IBenchmarker::WorkerResult Worker::wait() {
  runner_.join();
  return result_;
}

Worker::~Worker() {
  if (dD2H_) {
    runtime_.freeDevice(device_, dD2H_);
  }
  if (dH2D_) {
    runtime_.freeDevice(device_, dH2D_);
  }
  runtime_.destroyStream(stream_);
}