//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------
#include "common/MpOrchestrator.h"
#include "runtime/DeviceLayerFake.h"
#include "runtime/Types.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>

using namespace std::literals;
TEST(mp_only_orchestrator, 500_process) {
  MpOrchestrator orch;
  orch.createServer([] { return std::make_unique<dev::DeviceLayerFake>(); }, rt::Options{true, false});
  for (int i = 0; i < 500; ++i) {
    orch.createClient([](const rt::IRuntime*) {});
  }
}

TEST(mp_sync_events, wait_memcpy) {
  MpOrchestrator orch;
  orch.createServer([] { return std::make_unique<dev::DeviceLayerFake>(); }, rt::Options{true, false});
  for (int i = 0; i < 100; ++i) {
    orch.createClient([](rt::IRuntime* rt) {
      auto devices = rt->getDevices();
      ASSERT_FALSE(devices.empty());
      auto dev = devices[0];
      auto st = rt->createStream(dev);
      std::vector<std::byte> h_mem(1024);

      auto mem = rt->mallocDevice(dev, 1024);
      rt->memcpyHostToDevice(st, h_mem.data(), mem, 1024);
      rt->memcpyDeviceToHost(st, mem, h_mem.data(), 1024);
      rt->waitForStream(st);
    });
  }
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}