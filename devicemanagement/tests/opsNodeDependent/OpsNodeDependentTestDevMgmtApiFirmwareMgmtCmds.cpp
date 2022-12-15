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

class OpsNodeDependentTestDevMgmtApiFirmwareMgmtCmds : public TestDevMgmtApiSyncCmds {
  void SetUp() override {
    handle_ = dlopen("libDM.so", RTLD_LAZY);
    ASSERT_NE(handle_, nullptr);
  }
  void TearDown() override {
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
  }
};

TEST_F(OpsNodeDependentTestDevMgmtApiFirmwareMgmtCmds, resetMM) {
  if (isParallelRun()) {
    DV_LOG(INFO) << "Skipping the test since it cannot be run in parallel with ops device";
    return;
  }
  if (targetInList({Target::FullBoot, Target::FullChip, Target::Bemu, Target::Silicon})) {
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
    ASSERT_NE(devLayer_, nullptr);
    initTestTrace();
    controlTraceLogging();
    resetMM(false);
    extractAndPrintTraceData(false /* multiple devices */, TraceBufferType::TraceBufferSP);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
  }
}

TEST_F(OpsNodeDependentTestDevMgmtApiFirmwareMgmtCmds, resetMMWithOpsInUse) {
  if (isParallelRun()) {
    DV_LOG(INFO) << "Skipping the test since it cannot be run in parallel with ops device";
    return;
  }
  if (targetInList({Target::FullBoot, Target::FullChip, Target::Bemu, Target::Silicon})) {
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(true, true);
    ASSERT_NE(devLayer_, nullptr);
    initTestTrace();
    controlTraceLogging();
    resetMMWithOpsInUse(false);
    extractAndPrintTraceData(false /* multiple devices */, TraceBufferType::TraceBufferSP);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
  }
}

TEST_F(OpsNodeDependentTestDevMgmtApiFirmwareMgmtCmds, resetSOCSWithOpsInUse) {
  if (isParallelRun()) {
    DV_LOG(INFO) << "Skipping the test since it cannot be run in parallel with ops device";
    return;
  }
  devLayer_ = IDeviceLayer::createPcieDeviceLayer(true, true);
  ASSERT_NE(devLayer_, nullptr);
  initTestTrace();
  controlTraceLogging();
  resetSOCWithOpsInUse(false);
  extractAndPrintTraceData(false /* multiple devices */, TraceBufferType::TraceBufferSP);
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
