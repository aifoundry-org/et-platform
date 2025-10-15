//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------
#include "Utils.h"
#include "runtime/DeviceLayerFake.h"
#include "runtime/IRuntime.h"
#include <device-layer/DeviceLayerMock.h>
#include <gmock/gmock-actions.h>
#include <gtest/gtest.h>

using namespace rt;
using namespace testing;
using namespace std::chrono;

TEST(InitRuntime, HPSQ_Failure) {
  dev::DeviceLayerFake fake;
  auto deviceLayer = std::shared_ptr<NiceMock<dev::DeviceLayerMock>>(new NiceMock<dev::DeviceLayerMock>{&fake});
  deviceLayer->Delegate();
  ON_CALL(*deviceLayer.get(), sendCommandMasterMinion).WillByDefault(Return(false));
  EXPECT_THROW({ auto runtime = IRuntime::create(deviceLayer); }, Exception);
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  g3::log_levels::disable(DEBUG);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
