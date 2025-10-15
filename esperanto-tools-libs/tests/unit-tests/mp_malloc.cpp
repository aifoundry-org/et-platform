//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------
#include "common/MpOrchestrator.h"
#include "runtime/DeviceLayerFake.h"
#include "runtime/Types.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(mp_malloc, malloc_free_100clients_1000mallocs) {
  MpOrchestrator orch;
  orch.createServer([] { return std::make_unique<dev::DeviceLayerFake>(); }, rt::Options{true, false});
  auto dram = dev::DeviceLayerFake{}.getDramSize(0);
  for (int i = 0; i < 100; ++i) {
    orch.createClient([](rt::IRuntime* rt) {
      std::vector<std::byte*> allocs;
      auto dev = rt::DeviceId{0};
      for (auto j = 0; j < 1000; ++j) {
        allocs.emplace_back(rt->mallocDevice(dev, 1024));
      }
      for (auto a : allocs) {
        ASSERT_NE(a, nullptr);
        rt->freeDevice(dev, a);
      }
    });
  }
  orch.clearClients();
  orch.createClient([dram](rt::IRuntime* rt) { ASSERT_NO_THROW(rt->mallocDevice(rt::DeviceId{0}, dram - 3358720)); });
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
