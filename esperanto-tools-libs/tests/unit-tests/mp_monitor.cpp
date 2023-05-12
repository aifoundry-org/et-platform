//******************************************************************************
// Copyright (C) 2022, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------
#include "common/MpOrchestrator.h"
#include "runtime/DeviceLayerFake.h"
#include "runtime/IMonitor.h"
#include "runtime/Types.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logger.h>
#include <thread>

TEST(mp_monitor, getCurrentClients) {
  using namespace std::literals;
  MpOrchestrator orch;
  orch.createServer([] { return std::make_unique<dev::DeviceLayerFake>(); }, rt::Options{true, false});
  for (int i = 0; i < 100; ++i) {
    orch.createClient([](rt::IRuntime*) { std::this_thread::sleep_for(1s); });
  }
  auto monitor = rt::IMonitor::create(orch.getSocketPath());
  std::this_thread::sleep_for(500ms);
  auto clients = monitor->getCurrentClients();
  ASSERT_EQ(clients, 101); // 100 created clients + monitor
}

TEST(mp_monitor, getFreeMemory) {
  using namespace std::literals;
  MpOrchestrator orch;
  orch.createServer([] { return std::make_unique<dev::DeviceLayerFake>(); }, rt::Options{true, false});
  auto monitor = rt::IMonitor::create(orch.getSocketPath());
  auto initialMem = monitor->getFreeMemory()[rt::DeviceId(0)];
  constexpr auto allocSize = 1 << 12;
  ASSERT_LE(initialMem, dev::DeviceLayerFake{}.getDramSize(0));
  for (int i = 0; i < 10; ++i) {
    orch.createClient([](rt::IRuntime* rt) {
      rt->mallocDevice(rt::DeviceId{0}, allocSize);
      std::this_thread::sleep_for(1s);
    });
  }
  std::this_thread::sleep_for(500ms);
  auto currentMem = monitor->getFreeMemory()[rt::DeviceId(0)];
  ASSERT_EQ(currentMem, initialMem - 10 * allocSize);
  orch.clearClients();
  std::this_thread::sleep_for(1s);
  currentMem = monitor->getFreeMemory()[rt::DeviceId(0)];
  ASSERT_EQ(currentMem, initialMem);
}

TEST(mp_monitor, getAliveEvents) {
  using namespace std::literals;
  MpOrchestrator orch;
  orch.createServer([] { return std::make_unique<dev::DeviceLayerFake>(); }, rt::Options{true, false});
  auto monitor = rt::IMonitor::create(orch.getSocketPath());
  constexpr auto allocSize = (1 << 20) * 10;
  static std::vector<std::byte> h_src(allocSize);
  for (int i = 0; i < 10; ++i) {
    orch.createClient([](rt::IRuntime* rt) {
      constexpr auto dev = rt::DeviceId{0};
      auto monitor = static_cast<rt::IMonitor*>(static_cast<rt::Client*>(rt));
      auto dst = rt->mallocDevice(dev, allocSize);
      auto st = rt->createStream(dev);
      auto prevEvents = monitor->getAliveEvents()[dev];
      rt->memcpyHostToDevice(st, h_src.data(), dst, allocSize);
      auto currentEvents = monitor->getAliveEvents()[dev];
      ASSERT_GT(currentEvents, prevEvents);
      rt->waitForStream(st);
    });
  }
  orch.clearClients();
  ASSERT_EQ(0, monitor->getAliveEvents()[rt::DeviceId{0}]);
  ASSERT_EQ(0, monitor->getWaitingCommands()[rt::DeviceId{0}]);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}