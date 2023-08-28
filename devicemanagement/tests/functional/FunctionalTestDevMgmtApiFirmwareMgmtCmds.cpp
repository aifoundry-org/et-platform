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

class FunctionalTestDevMgmtApiFirmwareMgmtCmds : public TestDevMgmtApiSyncCmds {
  void SetUp() override {
    handle_ = dlopen("libDM.so", RTLD_LAZY);
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
    initTestTrace();
    controlTraceLogging();
    initDevErrorEvent();
  }
  void TearDown() override {
    // NOTE: Skip checking of device error events in ETSOC reset tests because error counters
    // are also reset during the reset
    extractAndPrintTraceData(false /* multiple devices */, TraceBufferType::TraceBufferSP);
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
  }
};

TEST_F(FunctionalTestDevMgmtApiFirmwareMgmtCmds, getMMErrorCount) {
  getMMErrorCount(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiFirmwareMgmtCmds, getFWBootstatus) {
  // TODO: SW-13807: Enable back on silicon after fix
  // if (targetInList({Target::FullBoot, Target::Silicon})) {
  if (targetInList({Target::FullBoot})) {
    getFWBootstatus(false /* Multiple devices */);
    checkDevErrorEvent();
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(FunctionalTestDevMgmtApiFirmwareMgmtCmds, getModuleFWRevision) {
  if (targetInList({Target::FullBoot, Target::Silicon})) {
    getModuleFWRevision(false /* Multiple devices */);
    checkDevErrorEvent();
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

/*TEST_F(FunctionalTestDevMgmtApiFirmwareMgmtCmds, setSpRootCertificate) {
  setSpRootCertificate(false);
}*/

// Pending SysEMU pointer update. Stuck behind a modelling bug
// TEST_F(FunctionalTestDevMgmtApiFirmwareMgmtCmds, updateFirmwareImage) {
//  updateFirmwareImage(false);
//}

TEST_F(FunctionalTestDevMgmtApiFirmwareMgmtCmds, resetSOCSingleDevice) {
  if (isParallelRun()) {
    DV_LOG(INFO) << "Skipping the test since it cannot be run in parallel with ops device";
    FLAGS_enable_trace_dump = false;
    return;
  }
  resetSOC(true /* Single Device */);
}

TEST_F(FunctionalTestDevMgmtApiFirmwareMgmtCmds, resetSOCMultiDevice) {
  if (isParallelRun()) {
    DV_LOG(INFO) << "Skipping the test since it cannot be run in parallel with ops device";
    FLAGS_enable_trace_dump = false;
    return;
  }
  resetSOC(false /* Multiple Devices */);
}

TEST_F(FunctionalTestDevMgmtApiFirmwareMgmtCmds, testShireCacheConfig) {
  if (targetInList({Target::FullBoot, Target::Silicon})) {
    testShireCacheConfig(false /* Multiple Devices */);
    checkDevErrorEvent();
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
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
