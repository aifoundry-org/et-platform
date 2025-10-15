//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------
#include "TestUtils.h"
#include "common/MpOrchestrator.h"
#include "runtime/Types.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(mp_memcpy, simpleSysemu) {
  MpOrchestrator orch;
  orch.createServer([] { return dev::IDeviceLayer::createSysEmuDeviceLayer(getSysemuDefaultOptions()); },
                    rt::Options{true, false});
  for (int i = 0; i < 100; ++i) {
    orch.createClient([](rt::IRuntime* rt) {
      auto devices = rt->getDevices();
      ASSERT_FALSE(devices.empty());
      auto dev = devices[0];
      auto st = rt->createStream(dev);
      std::vector<std::byte> h_src(1024);
      randomize(h_src, 0, 255);
      std::vector<std::byte> h_dst(1024);
      ASSERT_NE(h_src, h_dst);
      auto mem = rt->mallocDevice(dev, 1024);
      rt->memcpyHostToDevice(st, h_src.data(), mem, 1024);
      rt->memcpyDeviceToHost(st, mem, h_dst.data(), 1024);
      rt->waitForStream(st);
      ASSERT_EQ(h_src, h_dst);
    });
  }
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}