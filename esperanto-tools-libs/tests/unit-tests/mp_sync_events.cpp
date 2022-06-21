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
#include "runtime/Types.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

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

TEST(mp_sync_events, wait_kernel) {
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
      // TODO: when uncomment when https://esperantotech.atlassian.net/browse/SW-13022 is implemented
      // auto lc = rt->loadCode(st, h_mem.data(), 1024);
      rt->memcpyHostToDevice(st, h_mem.data(), mem, 1024);
      // TODO: when uncomment when https://esperantotech.atlassian.net/browse/SW-13022 is implemented
      // rt->kernelLaunch(st, lc.kernel_, nullptr, 0, 0x1);
      rt->memcpyDeviceToHost(st, mem, h_mem.data(), 1024);
      rt->waitForStream(st);
    });
  }
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}