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

class FunctionalTestDevMgmtApiPerfMgmtCmds : public TestDevMgmtApiSyncCmds {
  void SetUp() override {
    handle_ = dlopen("libDM.so", RTLD_LAZY);
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
    initTestTrace();
    controlTraceLogging(false);
  }
  void TearDown() override {
    extractAndPrintTraceData(false /* multiple devices */, TraceBufferType::TraceBufferSP);
    controlTraceLogging(true);
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
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
  logging::LoggerDefault loggerDefault_;
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
