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

class StressTestDevMgmtApiFirmwareMgmtCmds : public TestDevMgmtApiSyncCmds {
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

TEST_F(StressTestDevMgmtApiFirmwareMgmtCmds, updateFirmwareImageSingleDevice) {
  if (targetInList({Target::FullBoot, Target::Silicon})) {
    if (isParallelRun()) {
      DV_LOG(INFO) << "Skipping the test since it cannot be run in parallel with ops device";
      FLAGS_enable_trace_dump = false;
      return;
    }
    int iteration = 5;
    for (int i = 0; i < iteration; i++) {
      setFirmwareUpdateImage(true /* Single Device */, false);
    }
    extractAndPrintTraceData(true /* Single Device */, TraceBufferType::TraceBufferSP);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(StressTestDevMgmtApiFirmwareMgmtCmds, updateFirmwareImageMultiDevice) {
  if (targetInList({Target::FullBoot, Target::Silicon})) {
    if (isParallelRun()) {
      DV_LOG(INFO) << "Skipping the test since it cannot be run in parallel with ops device";
      FLAGS_enable_trace_dump = false;
      return;
    }
    int iteration = 5 / devLayer_->getDevicesCount();
    iteration = iteration ? iteration : 1;
    for (int i = 0; i < iteration; i++) {
      setFirmwareUpdateImage(false /* Multiple Devices */, false);
    }
    extractAndPrintTraceData(false /* Multiple Devices */, TraceBufferType::TraceBufferSP);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(StressTestDevMgmtApiFirmwareMgmtCmds, resetSOCSingleDevice) {
  if (isParallelRun()) {
    DV_LOG(INFO) << "Skipping the test since it cannot be run in parallel with ops device";
    return;
  }
  auto iteration = getTestTarget() == Target::Loopback ? 30 : 10;
  for (int i = 0; i < iteration; i++) {
    resetSOC(true /* Single Device */);
  }
  extractAndPrintTraceData(true /* Single Device */, TraceBufferType::TraceBufferSP);
}

TEST_F(StressTestDevMgmtApiFirmwareMgmtCmds, resetSOCMultiDevice) {
  if (isParallelRun()) {
    DV_LOG(INFO) << "Skipping the test since it cannot be run in parallel with ops device";
    return;
  }
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
