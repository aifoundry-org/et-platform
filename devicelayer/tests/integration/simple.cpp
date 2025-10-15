//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include <device-layer/DeviceLayerMock.h>
#include <device-layer/IDeviceLayer.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logging.h>

TEST(Simple, test_nothing) {
  ASSERT_TRUE(true);
}

TEST(Simple, mock_compiles) {
  [[maybe_unused]] dev::DeviceLayerMock mock;
}

TEST(VerboseLog, High) {
  ET_VLOG("CTEST", HIGH) << "VLOG_HIGH enabled";
}

TEST(VerboseLog, Mid) {
  ET_VLOG("CTEST", MID) << "VLOG_MID enabled";
}
TEST(VerboseLog, Low) {
  ET_VLOG("CTEST", LOW) << "VLOG_LOW enabled";
}

int main(int argc, char** argv) {
  logging::LoggerDefault loggerDefault_;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
