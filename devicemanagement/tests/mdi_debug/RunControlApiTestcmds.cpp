//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include "MDIApiTest.h"
#include "TestDevMgmtApiSyncCmds.h"
#include <dlfcn.h>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace dev;
using namespace device_management;

class RunControlApiTestcmds : public TestDevMgmtApiSyncCmds {
  void SetUp() override {
    initDMTestFramework();
  }
  void TearDown() override {
    cleanupDMTestFramework();
  }
};

TEST_F(RunControlApiTestcmds, testRunControlCmdsSetandUnsetBreakpoint) {
  if (targetInList({Target::Silicon, Target::SysEMU})) {
    testRunControlCmdsSetandUnsetBreakpoint(MDI_TEST_DEFAULT_SHIRE_ID, MDI_TEST_DEFAULT_THREAD_MASK,
                                            MDI_TEST_DEFAULT_HARTID,
                                            (COMPUTE_KERNEL_DEVICE_ADDRESS + MDI_TEST_BP_ADDRESS_OFFSET));
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(RunControlApiTestcmds, testRunControlCmdsGetHartStatus) {
  if (targetInList({Target::Silicon, Target::SysEMU})) {
    testRunControlCmdsGetHartStatus(MDI_TEST_DEFAULT_SHIRE_ID, MDI_TEST_DEFAULT_THREAD_MASK, MDI_TEST_DEFAULT_HARTID);
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
