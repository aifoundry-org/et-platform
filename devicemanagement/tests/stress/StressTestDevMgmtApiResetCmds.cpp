//******************************************************************************
// Copyright (C) 2022 Esperanto Technologies Inc.
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

class StressTestDevMgmtApiResetCmds : public TestDevMgmtApiSyncCmds {
  void SetUp() override {
    handle_ = dlopen("libDM.so", RTLD_LAZY);
    ASSERT_NE(handle_, nullptr);
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
    ASSERT_NE(devLayer_, nullptr);
    initTestTrace();
    controlTraceLogging();
  }
  void TearDown() override {
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
  }
};

TEST_F(StressTestDevMgmtApiResetCmds, resetSOCSingleDevice) {
  auto iteration = getTestTarget() == Target::Loopback ? 30 : 10;
  for (int i = 0; i < iteration; i++) {
    resetSOC(true /* Single Device */);
  }
  extractAndPrintTraceData(true /* Single Device */, TraceBufferType::TraceBufferSP);
}

TEST_F(StressTestDevMgmtApiResetCmds, resetSOCMultiDevice) {
  auto iteration = (getTestTarget() == Target::Loopback ? 30 : 10) / devLayer_->getDevicesCount();
  for (int i = 0; i < iteration; i++) {
    resetSOC(false /* Multiple Devices */);
  }
  extractAndPrintTraceData(false /* Multiple Devices */, TraceBufferType::TraceBufferSP);
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
