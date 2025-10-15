//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include "TestDevMgmtApiSyncCmds.h"
#include <dlfcn.h>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace dev;
using namespace device_management;

class FunctionalTestDevMgmtApiPerfMgmtCmds : public TestDevMgmtApiSyncCmds {
  void SetUp() override {
    initDMTestFramework();
  }
  void TearDown() override {
    cleanupDMTestFramework();
  }
};

TEST_F(FunctionalTestDevMgmtApiPerfMgmtCmds, getASICFrequencies) {
  getASICFrequencies(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiPerfMgmtCmds, getDRAMBW) {
  getDRAMBW(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiPerfMgmtCmds, getDRAMCapacityUtilization) {
  getDRAMCapacityUtilization(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiPerfMgmtCmds, getASICPerCoreDatapathUtilization) {
  getASICPerCoreDatapathUtilization(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiPerfMgmtCmds, getASICUtilization) {
  getASICUtilization(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiPerfMgmtCmds, getASICStalls) {
  getASICStalls(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiPerfMgmtCmds, getASICLatency) {
  getASICLatency(false /* Multiple devices */);
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
