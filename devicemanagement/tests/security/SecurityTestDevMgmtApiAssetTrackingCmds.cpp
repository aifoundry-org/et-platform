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

class SecurityTestDevMgmtApiAssetTrackingCmds : public TestDevMgmtApiSyncCmds {
  void SetUp() override {
    initDMTestFramework();
  }
  void TearDown() override {
    cleanupDMTestFramework();
  }
};

TEST_F(SecurityTestDevMgmtApiAssetTrackingCmds, getModuleManufactureNameInvalidOutputSize) {
  getModuleManufactureNameInvalidOutputSize(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiAssetTrackingCmds, getModuleManufactureNameInvalidDeviceNode) {
  getModuleManufactureNameInvalidDeviceNode(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiAssetTrackingCmds, getModuleManufactureNameInvalidHostLatency) {
  getModuleManufactureNameInvalidHostLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiAssetTrackingCmds, getModuleManufactureNameInvalidDeviceLatency) {
  getModuleManufactureNameInvalidDeviceLatency(false /* Multiple Devices */);
}

TEST_F(SecurityTestDevMgmtApiAssetTrackingCmds, getModuleManufactureNameInvalidOutputBuffer) {
  getModuleManufactureNameInvalidOutputBuffer(false /* Multiple Devices */);
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
