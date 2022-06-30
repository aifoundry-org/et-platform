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

class FunctionalTestDevMgmtApiAssetTrackingCmds : public TestDevMgmtApiSyncCmds {
  void SetUp() override {
    handle_ = dlopen("libDM.so", RTLD_LAZY);
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
    initTestTrace();
    controlTraceLogging();
  }
  void TearDown() override {
    extractAndPrintTraceData(false /* multiple devices */, TraceBufferType::TraceBufferSP);
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
  }
};

TEST_F(FunctionalTestDevMgmtApiAssetTrackingCmds, getModuleManufactureName) {
  if (targetInList({Target::FullBoot, Target::Silicon})) {
    getModuleManufactureName(false /* Multiple devices */);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(FunctionalTestDevMgmtApiAssetTrackingCmds, getModulePartNumber) {
  if (targetInList({Target::FullBoot, Target::Silicon})) {
    getModulePartNumber(false /* Multiple devices */);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(FunctionalTestDevMgmtApiAssetTrackingCmds, getModuleSerialNumber) {
  // TODO: SW-13220: Enable back on Target::Silicon, following failure is seen with V2/V3 card
  // received: 0x487811C8, expected: 0x12345678
  // if (targetInList({Target::FullBoot, Target::Silicon})) {
  if (targetInList({Target::FullBoot})) {
    getModuleSerialNumber(false /* Multiple devices */);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(FunctionalTestDevMgmtApiAssetTrackingCmds, getASICChipRevision) {
  // TODO: SW-13220: Enable back on Target::Silicon, following failure is seen with V2/V3 card
  // received: 255, expected: 160
  // if (targetInList({Target::FullBoot, Target::FullChip, Target::Bemu, Target::Silicon})) {
  if (targetInList({Target::FullBoot, Target::FullChip, Target::Bemu})) {
    getASICChipRevision(false /* Multiple devices */);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(FunctionalTestDevMgmtApiAssetTrackingCmds, getModulePCIENumPortsMaxSpeed) {
  // TODO: SW-13220: Enable back on Target::Silicon, following failure is seen with V2/V3 card
  // received: 5, expected: 8, need max speed not current speed, also rename to
  // getModulePCIEPortsMaxSpeed
  // Does not fail on all V3 cards, not passing on: card on slot 0000:05:00.0 (mv-swpcie25)
  if (getTestTarget() != Target::Silicon) {
    getModulePCIENumPortsMaxSpeed(false /* Multiple devices */);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(FunctionalTestDevMgmtApiAssetTrackingCmds, getModuleMemorySizeMB) {
  if (targetInList({Target::FullBoot, Target::Silicon})) {
    getModuleMemorySizeMB(false /* Multiple devices */);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(FunctionalTestDevMgmtApiAssetTrackingCmds, getModuleRevision) {
  if (targetInList({Target::FullBoot, Target::Silicon})) {
    getModuleRevision(false /* Multiple devices */);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(FunctionalTestDevMgmtApiAssetTrackingCmds, getModuleFormFactor) {
  if (targetInList({Target::FullBoot, Target::Silicon})) {
    getModuleFormFactor(false /* Multiple devices */);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(FunctionalTestDevMgmtApiAssetTrackingCmds, getModuleMemoryVendorPartNumber) {
  getModuleMemoryVendorPartNumber(false /* Multiple devices */);
}

TEST_F(FunctionalTestDevMgmtApiAssetTrackingCmds, getModuleMemoryType) {
  getModuleMemoryType(false /* Multiple devices */);
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
