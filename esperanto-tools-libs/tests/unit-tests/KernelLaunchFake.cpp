//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------
#include "Utils.h"
#include "runtime/IRuntime.h"
#include <chrono>
#include <device-layer/IDeviceLayerFake.h>
#include <functional>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logger.h>
#include <thread>

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wkeyword-macro"
#endif
#define private public
#pragma GCC diagnostic pop
#include "RuntimeImp.h"
#undef private

using namespace rt;
using namespace testing;
using namespace std::chrono;
struct KernelLaunchF : Test {
  void SetUp() override {
    runtime_ = rt::IRuntime::create(&deviceLayer_);
    device_ = runtime_->getDevices()[0];
    stream_ = runtime_->createStream(device_);
    auto rt = static_cast<RuntimeImp*>(runtime_.get());
    rt->setMemoryManagerDebugMode(runtime_->getDevices()[0], true);
    rt->kernels_.insert({KernelId{0}, std::make_unique<RuntimeImp::Kernel>(device_, nullptr, 0x4000)});
  }
  void TearDown() override {
    runtime_->destroyStream(stream_);
  }
  void sendH2D_K_D2H(int iterations, size_t args_size, size_t transferSize) {
    dummy_.resize(std::max(args_size, transferSize));
    for (int i = 0; i < iterations; ++i) {
      runtime_->memcpyHostToDevice(stream_, dummy_.data(), nullptr, transferSize);
      runtime_->kernelLaunch(stream_, kernel_, dummy_.data(), args_size, 0x3);
      runtime_->memcpyDeviceToHost(stream_, nullptr, dummy_.data(), transferSize);
    }
    runtime_->waitForStream(stream_);
  }
  dev::IDeviceLayerFake deviceLayer_;
  std::vector<std::byte> dummy_;
  RuntimePtr runtime_;
  KernelId kernel_{0};
  StreamId stream_;
  DeviceId device_;
};

TEST_F(KernelLaunchF, simple) {
  sendH2D_K_D2H(1, 64, 1024);
}

TEST_F(KernelLaunchF, largerKernelArgs) {
  sendH2D_K_D2H(1, 128, 1024);
}

TEST_F(KernelLaunchF, largerKernelArgs_thousands_iters) {
  sendH2D_K_D2H(5000, 128, 1024);
}

TEST_F(KernelLaunchF, largeTransfer) {
  sendH2D_K_D2H(10, 64, 512ULL * (1ULL << 20));
}

TEST_F(KernelLaunchF, largeTransferAndLargeArgs) {
  sendH2D_K_D2H(10, 128, 512ULL * (1ULL << 20));
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  g3::log_levels::disable(DEBUG);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}