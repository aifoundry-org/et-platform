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
    // NOTE: DM Event processor cannot run during ETSOC reset
    // TODO: SW-18858: Add support to stop and restart event processor for ETSOC
    // reset keeping the option to run the event processor threads in detach mode
  }
  void TearDown() override {
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
  }
};

/* TODO: SW-19075: Re-enable once issue is resolved */
TEST_F(StressTestDevMgmtApiFirmwareMgmtCmds, DISABLED_backToBackUpdateFirmwareImageSingleDevice) {
  if (targetInList({Target::FullBoot, Target::Silicon})) {
    if (isParallelRun()) {
      DV_LOG(INFO) << "Skipping the test since it cannot be run in parallel with ops device";
      FLAGS_enable_trace_dump = false;
      return;
    }
    initEventProcessor();
    setFirmwareUpdateImage(true /* Single Device */, false, 4);
    cleanupEventProcessor();
    extractAndPrintTraceData(true /* Single Device */, TraceBufferType::TraceBufferSP);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

/* TODO: SW-19075: Re-enable once issue is resolved */
TEST_F(StressTestDevMgmtApiFirmwareMgmtCmds, DISABLED_backToBackUpdateFirmwareImageMultiDevice) {
  if (targetInList({Target::FullBoot, Target::Silicon})) {
    if (isParallelRun()) {
      DV_LOG(INFO) << "Skipping the test since it cannot be run in parallel with ops device";
      FLAGS_enable_trace_dump = false;
      return;
    }
    initEventProcessor();
    int iterations = 4 / devLayer_->getDevicesCount();
    iterations = iterations ? iterations : 1;
    setFirmwareUpdateImage(false /* Multiple Devices */, false, iterations);
    cleanupEventProcessor();
    extractAndPrintTraceData(false /* Multiple Devices */, TraceBufferType::TraceBufferSP);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(StressTestDevMgmtApiFirmwareMgmtCmds, DISABLED_backToBackUpdateFirmwareImageAndResetSingleDevice) {
  if (targetInList({Target::FullBoot, Target::Silicon})) {
    if (isParallelRun()) {
      DV_LOG(INFO) << "Skipping the test since it cannot be run in parallel with ops device";
      FLAGS_enable_trace_dump = false;
      return;
    }
    initEventProcessor();
    setFirmwareUpdateImage(true /* Single Device */, true, 4);
    cleanupEventProcessor();
    extractAndPrintTraceData(true /* Single Device */, TraceBufferType::TraceBufferSP);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(StressTestDevMgmtApiFirmwareMgmtCmds, DISABLED_backToBackUpdateFirmwareImageAndResetMultiDevice) {
  if (targetInList({Target::FullBoot, Target::Silicon})) {
    if (isParallelRun()) {
      DV_LOG(INFO) << "Skipping the test since it cannot be run in parallel with ops device";
      FLAGS_enable_trace_dump = false;
      return;
    }
    initEventProcessor();
    int iterations = 4 / devLayer_->getDevicesCount();
    iterations = iterations ? iterations : 1;
    setFirmwareUpdateImage(false /* Multiple Devices */, true, iterations);
    cleanupEventProcessor();
    extractAndPrintTraceData(false /* Multiple Devices */, TraceBufferType::TraceBufferSP);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(StressTestDevMgmtApiFirmwareMgmtCmds, backToBackResetSOCSingleDevice) {
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

TEST_F(StressTestDevMgmtApiFirmwareMgmtCmds, backToBackResetSOCMultiDevice) {
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
