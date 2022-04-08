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
    // TODO: SW-10585: Enable back these tests on silicon currently service not functional
    if (getTestTarget() == Target::Silicon) {
      std::exit(EXIT_SUCCESS);
    }
    handle_ = dlopen("libDM.so", RTLD_LAZY);
    devLayer_ = IDeviceLayer::createPcieDeviceLayer(false, true);
    initTestTrace();
    controlTraceLogging(true);
  }
  void TearDown() override {
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
  if (targetInList({Target::FullBoot, Target::Silicon})) {
    getFWBootstatus(false /* Multiple devices */);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(FunctionalTestDevMgmtApiFirmwareMgmtCmds, getModuleFWRevision) {
  if (targetInList({Target::FullBoot, Target::Silicon})) {
    getModuleFWRevision(false /* Multiple devices */);
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

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
