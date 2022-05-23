//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------
#if 0
//#TODO COMMENTED TEST UNTIL WE GET MERGED jvera/SW-12621-remove-devicelayer-fake in SW-PLATFORM
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
  auto deviceLayer = NiceMock<dev::DeviceLayerMock>{&fake};
  deviceLayer.Delegate();
  ON_CALL(deviceLayer, sendCommandMasterMinion).WillByDefault(Return(false));
  EXPECT_THROW({ auto runtime = IRuntime::create(&deviceLayer); }, Exception);
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  g3::log_levels::disable(DEBUG);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
#endif