//******************************************************************************
// Copyright (C) 2021 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "MDIApiTest.h"
#include "TestDevMgmtApiSyncCmds.h"
#include <dlfcn.h>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace dev;
using namespace device_management;

class StateInspectionApiTestcmds : public TestDevMgmtApiSyncCmds {
  void SetUp() override {
    initDMTestFramework();
  }
  void TearDown() override {
    cleanupDMTestFramework();
  }
};

TEST_F(StateInspectionApiTestcmds, readMem_unprivileged) {
  if (targetInList({Target::Silicon, Target::SysEMU})) {
    readMem_unprivileged(COMPUTE_KERNEL_DEVICE_ADDRESS);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(StateInspectionApiTestcmds, readMem_privileged) {
  if (targetInList({Target::Silicon, Target::SysEMU})) {
    addToDevErrorEventSkipList({DevErrorEvent::EventType::SpCeEvent});
    readMem_privileged(MM_FW_MASTER_SDATA_BASE);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(StateInspectionApiTestcmds, writeMem_unprivileged) {
  if (targetInList({Target::Silicon, Target::SysEMU})) {
    writeMem_unprivileged(MDI_TEST_WRITE_MEM_TEST_DATA, COMPUTE_KERNEL_DEVICE_ADDRESS);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(StateInspectionApiTestcmds, writeMem_privileged) {
  if (targetInList({Target::Silicon, Target::SysEMU})) {
    addToDevErrorEventSkipList({DevErrorEvent::EventType::SpCeEvent});
    writeMem_privileged(MDI_TEST_WRITE_MEM_TEST_DATA, MM_FW_MASTER_SDATA_BASE);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(StateInspectionApiTestcmds, testStateInspectionReadGPR) {
  if (targetInList({Target::Silicon, Target::SysEMU})) {
    testStateInspectionReadGPR(MDI_TEST_DEFAULT_SHIRE_ID, MDI_TEST_DEFAULT_THREAD_MASK, MDI_TEST_DEFAULT_HARTID);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(StateInspectionApiTestcmds, testStateInspectionWriteGPR) {
  if (targetInList({Target::Silicon, Target::SysEMU})) {
    testStateInspectionWriteGPR(MDI_TEST_DEFAULT_SHIRE_ID, MDI_TEST_DEFAULT_THREAD_MASK, MDI_TEST_DEFAULT_HARTID,
                                MDI_TEST_GPR_WRITE_TEST_DATA);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(StateInspectionApiTestcmds, testStateInspectionReadCSR) {
  if (targetInList({Target::Silicon, Target::SysEMU})) {
    testStateInspectionReadCSR(MDI_TEST_DEFAULT_SHIRE_ID, MDI_TEST_DEFAULT_THREAD_MASK, MDI_TEST_DEFAULT_HARTID,
                               MDI_TEST_CSR_PC_REG);
  } else {
    DV_LOG(INFO) << "Skipping the test since its not supported on current target";
    FLAGS_enable_trace_dump = false;
  }
}

TEST_F(StateInspectionApiTestcmds, testStateInspectionWriteCSR) {
  if (targetInList({Target::Silicon, Target::SysEMU})) {
    testStateInspectionWriteCSR(MDI_TEST_DEFAULT_SHIRE_ID, MDI_TEST_DEFAULT_THREAD_MASK, MDI_TEST_DEFAULT_HARTID,
                                MDI_TEST_CSR_PC_REG, MDI_TEST_CSR_WRITE_PC_TEST_ADDRESS);
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
