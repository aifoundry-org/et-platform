//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------
#include "KernelLaunchOptionsImp.h"
#include "Utils.h"
#include "runtime/DeviceLayerFake.h"
#include "runtime/IRuntime.h"
#include "runtime/Types.h"
#include <chrono>
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
    runtime_ = rt::IRuntime::create(&deviceLayer_, rt::Options{false, false});
    runtime_->setOnStreamErrorsCallback([](auto, const auto&) { FAIL(); });
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

  void send_K(int iterations, size_t args_size, bool waitIter = false) {
    dummy_.resize(args_size);
    for (int i = 0; i < iterations; ++i) {
      runtime_->kernelLaunch(stream_, kernel_, dummy_.data(), args_size, 0x3);
      if (waitIter) {
        runtime_->waitForStream(stream_);
      }
    }
    runtime_->waitForStream(stream_);
  }

  void sendH2D_K_D2H_WithOptions(int iterations, size_t args_size, size_t transferSize, KernelLaunchOptions& opts) {
    dummy_.resize(std::max(args_size, transferSize));
    for (int i = 0; i < iterations; ++i) {
      runtime_->memcpyHostToDevice(stream_, dummy_.data(), nullptr, transferSize);
      runtime_->kernelLaunch(stream_, kernel_, dummy_.data(), args_size, opts);
      runtime_->memcpyDeviceToHost(stream_, nullptr, dummy_.data(), transferSize);
    }
    runtime_->waitForStream(stream_);
  }

  dev::DeviceLayerFake deviceLayer_;
  std::vector<std::byte> dummy_;
  RuntimePtr runtime_;
  KernelId kernel_{0};
  StreamId stream_;
  DeviceId device_;
};

TEST_F(KernelLaunchF, simple) {
  sendH2D_K_D2H(1, 64, 1024);
}

TEST_F(KernelLaunchF, embedKernelArgs) {
  sendH2D_K_D2H(1, 16, 1024);
}

TEST_F(KernelLaunchF, largerKernelArgs) {
  sendH2D_K_D2H(1, 128, 1024);
}

TEST_F(KernelLaunchF, largerKernelArgs_thousands_iters) {
  sendH2D_K_D2H(5000, 128, 1024);
}

TEST_F(KernelLaunchF, largeTransfer) {
  sendH2D_K_D2H(10, 64, 512ULL << 20);
}

TEST_F(KernelLaunchF, largeTransferAndLargeArgs) {
  sendH2D_K_D2H(10, 128, 512ULL << 20);
}

TEST_F(KernelLaunchF, onlyKernels10K) {
  send_K(1e4, 32);
}

TEST_F(KernelLaunchF, onlyKernels10K_waitIters) {
  send_K(1e4, 32, true);
}

TEST_F(KernelLaunchF, simpleOptions) {
  constexpr size_t kTraceBytesPerHart = 4096;
  constexpr size_t kNumHarts = 2048;
  constexpr size_t kTraceBufferSize = kTraceBytesPerHart * kNumHarts;
  constexpr uint32_t TRACE_EVENT_ENABLE_ALL = 0xFFFFFFFFU;
  constexpr uint32_t TRACE_FILTER_ENABLE_ALL = 0xFFFFFFFFU;

  KernelLaunchOptions opts;
  opts.setShireMask(0x3);
  opts.setBarrier(true);
  opts.setFlushL3(false);

  std::byte* addrptr = runtime_->mallocDevice(device_, kTraceBufferSize);
  uint32_t threshold = 0;
  uint64_t shireMask = 0x3FULL;
  uint64_t threadMask = 0xFFFFFFFFFFFFFFFFULL;
  uint32_t eventMask = TRACE_EVENT_ENABLE_ALL;
  uint32_t filterMask = TRACE_FILTER_ENABLE_ALL;

  opts.setUserTracing(reinterpret_cast<uint64_t>(addrptr), kTraceBufferSize, threshold, shireMask, threadMask,
                      eventMask, filterMask);

  opts.setCoreDumpFilePath("Tracefile.bin");

  sendH2D_K_D2H_WithOptions(1, 64, 1024, opts);
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  g3::log_levels::disable(DEBUG);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
