//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include "runtime/DeviceLayerFake.h"
#include <ios>
#include <sstream>

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wkeyword-macro"
#endif
#define private public
#pragma GCC diagnostic pop
#include "MemoryManager.h"
#include "RuntimeImp.h"
#include "common/Constants.h"
#include "runtime/IRuntime.h"
#undef private
#include <chrono>
#include <cstdio>
#include <experimental/filesystem>
#include <gtest/gtest.h>

using namespace rt;

TEST(StreamsLifeCycle, simple) {
  auto deviceLayer = std::make_shared<dev::DeviceLayerFake>();
  auto runtime = rt::IRuntime::create(deviceLayer, Options{true, false});
  runtime->setOnStreamErrorsCallback([](auto, const auto&) { FAIL(); });
  auto devices = runtime->getDevices();
  ASSERT_GE(devices.size(), 0);
  EXPECT_NO_THROW({
    auto streamId = runtime->createStream(devices[0]);
    runtime->destroyStream(streamId);
  });
}

TEST(StreamsLifeCycle, create_and_destroy_10k_streams) {
  auto deviceLayer = std::make_shared<dev::DeviceLayerFake>();
  auto runtime = rt::IRuntime::create(deviceLayer, Options{true, false});
  runtime->setOnStreamErrorsCallback([](auto, const auto&) { FAIL(); });
  auto devices = runtime->getDevices();
  ASSERT_GE(devices.size(), 0);
  std::vector<StreamId> streams_;
  for (auto i = 0U; i < 10000; ++i) {
    streams_.emplace_back(runtime->createStream(devices[0]));
    if (i > 0) {
      EXPECT_GT(static_cast<uint32_t>(streams_[i]), static_cast<uint32_t>(streams_[i - 1]));
    }
  }
  // check these are the same as inside the runtime
  auto rt = static_cast<RuntimeImp*>(runtime.get());
  EXPECT_EQ(rt->streamManager_.streams_.size(), streams_.size());
  for (auto s : streams_) {
    EXPECT_NE(rt->streamManager_.streams_.find(s), end(rt->streamManager_.streams_));
  }
  // destroy all streams and expect there are no streams inside the runtime
  for (auto s : streams_) {
    runtime->destroyStream(s);
  }
  EXPECT_TRUE(rt->streamManager_.streams_.empty());
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
