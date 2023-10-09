//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestDevMgmtApiSyncCmds.h"
#include <dlfcn.h>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace dev;
using namespace device_management;

class FunctionalTestDevMgmtApiErrorControlCmds : public TestDevMgmtApiSyncCmds {
  void SetUp() override {
    initDMTestFramework();
  }
  void TearDown() override {
    cleanupDMTestFramework();
  }
};

TEST_F(FunctionalTestDevMgmtApiErrorControlCmds, setAndGetDDRECCThresholdCount) {
  setAndGetDDRECCThresholdCount(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiErrorControlCmds, setAndGetSRAMECCThresholdCount) {
  setAndGetSRAMECCThresholdCount(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiErrorControlCmds, setAndGetPCIEECCThresholdCount) {
  setAndGetPCIEECCThresholdCount(false /* Multiple devices */);
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
