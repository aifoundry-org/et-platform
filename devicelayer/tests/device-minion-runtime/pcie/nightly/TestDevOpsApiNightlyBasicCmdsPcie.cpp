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
 * Test Labels: NIGHTLY, PCIE, OPS, FUNCTIONAL, SYSTEM
 */

class TestDevOpsApiNightlyBasicCmdsPcie : public TestDevOpsApiBasicCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);

    initTestHelperPcie();
  }
};

TEST_F(TestDevOpsApiNightlyBasicCmdsPcie, backToBackSame3kCmdsSingleDeviceSingleQueue_1_1) {
  bool singleDevice = true;
  bool singleQueue = true;
  backToBackSameCmds(singleDevice, singleQueue, 3000);
}

TEST_F(TestDevOpsApiNightlyBasicCmdsPcie, backToBackSame3kCmdsSingleDeviceMultiQueue_1_2) {
  bool singleDevice = true;
  bool singleQueue = false;
  backToBackSameCmds(singleDevice, singleQueue, 3000);
}

TEST_F(TestDevOpsApiNightlyBasicCmdsPcie, backToBackDiff3kCmdsSingleDeviceSingleQueue_1_3) {
  bool singleDevice = true;
  bool singleQueue = true;
  backToBackDiffCmds(singleDevice, singleQueue, 3000);
}

TEST_F(TestDevOpsApiNightlyBasicCmdsPcie, backToBackDiff3kCmdsSingleDeviceMultiQueue_1_4) {
  bool singleDevice = true;
  bool singleQueue = false;
  backToBackDiffCmds(singleDevice, singleQueue, 3000);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
