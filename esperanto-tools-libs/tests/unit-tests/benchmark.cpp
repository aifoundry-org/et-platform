//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------
#include "runtime/IRuntime.h"
#include "utils.h"
#include <chrono>
#include <hostUtils/logging/Logger.h>
#include <device-layer/IDeviceLayerFake.h>
#include <functional>
#include <gtest/gtest.h>
#include <thread>

using namespace rt;
using namespace testing;
using namespace std::chrono;

#define private public
#include "RuntimeImp.h"
#undef private

struct Chrono {
  void start() {
    start_ = high_resolution_clock::now();
  }
  void stop() {
    stop_ = high_resolution_clock::now();
  }

  float getSeconds() const {
    auto us = duration_cast<microseconds>(stop_ - start_);
    return us.count() / 1e6f;
  }

  void printMsg(const std::string msg) const {
    auto duration = stop_ - start_;
    auto h = duration_cast<hours>(duration);
    auto m = duration_cast<minutes>(duration -= h);
    auto s = duration_cast<seconds>(duration -= m);
    auto ms = duration_cast<milliseconds>(duration -= s);
    auto us = duration_cast<microseconds>(duration -= ms);
  }

  time_point<high_resolution_clock> start_;
  time_point<high_resolution_clock> stop_;
};

struct RuntimeBenchmark : Test {
  void SetUp() override {
    runtime_ = rt::IRuntime::create(&deviceLayer_);
    device_ = runtime_->getDevices()[0];
    stream_ = runtime_->createStream(device_);
    static_cast<RuntimeImp*>(runtime_.get())
      ->kernels_.insert({KernelId{0}, std::make_unique<RuntimeImp::Kernel>(device_, nullptr, 0x4000)});
  }
  void TearDown() override {
    runtime_->destroyStream(stream_);
  }
  dev::IDeviceLayerFake deviceLayer_;
  RuntimePtr runtime_;
  KernelId kernel_{0};
  StreamId stream_;
  DeviceId device_;
};

void sendDmas(int num_dmas, StreamId stream, RuntimePtr& runtime) {
  RT_LOG(INFO) << "Submitting " << num_dmas << " DMA commands";
  const size_t transferSize = 16;
  std::byte dummyBuffer[transferSize];
  for (int i = 0; i < num_dmas; ++i) {
    runtime->memcpyHostToDevice(stream, dummyBuffer, nullptr, transferSize);
  }
  RT_LOG(INFO) << "Waiting for all commands processed";
  runtime->waitForStream(stream);
}

TEST_F(RuntimeBenchmark, DMA_transactions) {
  // sendDmas(1e6, stream_, runtime_);
  // sendDmas(1e4, stream_, runtime_);
  sendDmas(1e3, stream_, runtime_);
  // sendDmas(1e6, stream_, runtime_);
}

void sendH2D_K_D2H(int iterations, StreamId stream, KernelId kernel, RuntimePtr& runtime, bool sync_on_each_iter) {
  RT_LOG(INFO) << "Submitting " << iterations
               << " HostToDevice DMA + Kernel + DeviceToHost DMA. Syncing on each iteration? "
               << (sync_on_each_iter ? "True" : "False");
  char args[128];
  const size_t transferSize = 16;
  std::byte dummyBuffer[transferSize];
  for (int i = 0; i < iterations; ++i) {
    runtime->memcpyHostToDevice(stream, dummyBuffer, nullptr, transferSize);
    runtime->kernelLaunch(stream, kernel, args, sizeof args, 0x3);
    runtime->memcpyDeviceToHost(stream, nullptr, dummyBuffer, transferSize);
    if (sync_on_each_iter) {
      runtime->waitForStream(stream);
    }
  }
}
TEST_F(RuntimeBenchmark, H2D_K_D2H_Sync_On_Iters) {
  sendH2D_K_D2H(1e2, stream_, kernel_, runtime_, true);
  sendH2D_K_D2H(1e3, stream_, kernel_, runtime_, true);
}

TEST_F(RuntimeBenchmark, H2D_K_D2H_NoSync_On_Iters) {
  sendH2D_K_D2H(1e2, stream_, kernel_, runtime_, false);
  runtime_->waitForStream(stream_);
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  g3::log_levels::disable(DEBUG);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}