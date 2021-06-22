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

class TestDevMgmtApiFuncSyncCmdsBemu : public TestDevMgmtApiSyncCmds {
public:
  void SetUp() override {
    handle_ = dlopen("libDM.so", RTLD_LAZY);
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
  }

  void TearDown() override {
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
  }
};

TEST_F(TestDevMgmtApiFuncSyncCmdsBemu, getModuleManufactureName_1_1) {
  getModuleManufactureName_1_1(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsBemu, getASICChipRevision_1_4) {
  getASICChipRevision_1_4(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsBemu, getModulePower_1_16) {
  getModulePower_1_16(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsBemu, getModuleVoltage_1_17) {
  getModuleVoltage_1_17(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsBemu, getModuleCurrentTemperature_1_18) {
  getModuleCurrentTemperature_1_18(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsBemu, getModuleMaxDDRBW_1_21) {
  getModuleMaxDDRBW_1_21(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsBemu, getDDRBWCounter_1_29) {
  getDDRBWCounter_1_29(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsBemu, getDRAMBW_1_34) {
  getDRAMBW_1_34(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsBemu, getDRAMCapacityUtilization_1_35) {
  getDRAMCapacityUtilization_1_35(false /* Multiple devices */);
}

TEST_F(TestDevMgmtApiFuncSyncCmdsBemu, getDeviceErrorEvents_1_44) {
  getDeviceErrorEvents_1_44(false /* Multiple devices */);
}

int main(int argc, char** argv) {
  logging::LoggerDefault loggerDefault_;
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
