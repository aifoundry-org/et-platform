//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include <deviceLayer/IDeviceLayer.h>
#include <deviceLayer/IDeviceLayerMock.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logging.h>

TEST(Simple, test_nothing) {
  ASSERT_TRUE(true);
}

TEST(Simple, mock_compiles) {
  [[maybe_unused]] dev::IDeviceLayerMock mock;
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
