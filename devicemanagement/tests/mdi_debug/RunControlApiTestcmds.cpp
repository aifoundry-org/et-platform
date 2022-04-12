//******************************************************************************
// Copyright (C) 2021 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestDevMgmtApiSyncCmds.h"
#include "MDIApiTest.h"
#include <dlfcn.h>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace dev;
using namespace device_management;

class RunControlApiTestcmds : public TestDevMgmtApiSyncCmds {
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

TEST_F(RunControlApiTestcmds, testRunControlCmdsSetandUnsetBreakpoint) {
  if (targetInList({Target::Silicon, Target::SysEMU})) {
    testRunControlCmdsSetandUnsetBreakpoint(MDI_TEST_DEFAULT_SHIRE_ID, 
             MDI_TEST_DEFAULT_THREAD_MASK, MDI_TEST_DEFAULT_HARTID, (COMPUTE_KERNEL_DEVICE_ADDRESS+MDI_TEST_BP_ADDRESS_OFFSET));
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(RunControlApiTestcmds, testRunControlCmdsGetHartStatus) {
  if (targetInList({Target::Silicon, Target::SysEMU})) {
    testRunControlCmdsGetHartStatus(MDI_TEST_DEFAULT_SHIRE_ID, 
             MDI_TEST_DEFAULT_THREAD_MASK, MDI_TEST_DEFAULT_HARTID);
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
