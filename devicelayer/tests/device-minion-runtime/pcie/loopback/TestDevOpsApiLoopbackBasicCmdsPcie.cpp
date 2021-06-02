//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestDevOpsApiBasicCmds.h"

/*
 * Test Labels: PCIE, OPS, FUNCTIONAL, SYSTEM
 */

class TestDevOpsApiLoopbackBasicCmdsPcie : public TestDevOpsApiBasicCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);

    initTestHelperPcie();
  }
};

TEST_F(TestDevOpsApiLoopbackBasicCmdsPcie, backToBackSame50kCmdsSingleDeviceSingleQueue_1_1) {
  bool singleDevice = true;
  bool singleQueue = true;
  backToBackSameCmds(singleDevice, singleDevice, 50000);
}

TEST_F(TestDevOpsApiLoopbackBasicCmdsPcie, backToBackSame50kCmdsSingleDeviceMultiQueue_1_2) {
  bool singleDevice = true;
  bool singleQueue = false;
  backToBackSameCmds(singleDevice, singleDevice, 50000);
}

TEST_F(TestDevOpsApiLoopbackBasicCmdsPcie, backToBackSame50kCmdsMultiDeviceMultiQueue_1_3) {
  bool singleDevice = false;
  bool singleQueue = false;
  backToBackSameCmds(singleDevice, singleDevice, 50000);
}

TEST_F(TestDevOpsApiLoopbackBasicCmdsPcie, backToBackDiff50kCmdsSingleDeviceSingleQueue_1_4) {
  bool singleDevice = true;
  bool singleQueue = true;
  backToBackDiffCmds(singleDevice, singleDevice, 50000);
}

TEST_F(TestDevOpsApiLoopbackBasicCmdsPcie, backToBackDiff50kCmdsSingleDeviceMultiQueue_1_5) {
  bool singleDevice = true;
  bool singleQueue = false;
  backToBackDiffCmds(singleDevice, singleDevice, 50000);
}

TEST_F(TestDevOpsApiLoopbackBasicCmdsPcie, backToBackDiff50kCmdsMultiDeviceMultiQueue_1_6) {
  bool singleDevice = false;
  bool singleQueue = false;
  backToBackDiffCmds(singleDevice, singleDevice, 50000);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
