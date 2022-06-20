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

TEST(mp_createdestroy_streams, create_destroy_1000_streams) {
  auto dl = std::make_unique<dev::DeviceLayerFake>();
  MpOrchestrator orch(std::move(dl), rt::Options{true, false});
  for (int i = 0; i < 25; ++i) {
    orch.createClient([](rt::IRuntime* rt) {
      std::vector<rt::StreamId> streams;
      auto devices = rt->getDevices();
      ASSERT_FALSE(devices.empty());
      auto dev = devices[0];
      for (auto j = 0; j < 25; ++j) {
        ASSERT_NO_THROW(streams.emplace_back(rt->createStream(dev)););
      }
      for (auto s : streams) {
        ASSERT_NO_THROW(rt->destroyStream(s););
      }
      for (auto s : streams) {
        ASSERT_THROW(rt->destroyStream(s), rt::Exception);
      }
    });
  }
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}