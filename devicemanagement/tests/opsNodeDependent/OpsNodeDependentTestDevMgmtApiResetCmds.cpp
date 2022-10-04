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

class OpsNodeDependentTestDevMgmtApiResetCmds : public TestDevMgmtApiSyncCmds {
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

TEST_F(OpsNodeDependentTestDevMgmtApiResetCmds, resetMM) {
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

TEST_F(OpsNodeDependentTestDevMgmtApiResetCmds, resetMMInvalidOpsNodePermission) {
  if (targetInList({Target::FullBoot, Target::FullChip, Target::Bemu, Target::Silicon})) {
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(true, true);
    ASSERT_NE(devLayer_, nullptr);
    initTestTrace();
    controlTraceLogging();
    resetMMOpsOpen(false);
    extractAndPrintTraceData(false /* multiple devices */, TraceBufferType::TraceBufferSP);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
  }
}

TEST_F(OpsNodeDependentTestDevMgmtApiResetCmds, resetSOC) {
  if (targetInList({Target::Silicon, Target::SysEMU, Target::Loopback})) {
    auto initialize = [&](void) {
      devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
      ASSERT_NE(devLayer_, nullptr);
      initTestTrace();
      controlTraceLogging();
    };
    auto destroy = [&](void) {
      if (devLayer_) {
        devLayer_.reset();
      }
    };
    resetSOC(false /* Multiple Devices */, initialize, destroy);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
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
