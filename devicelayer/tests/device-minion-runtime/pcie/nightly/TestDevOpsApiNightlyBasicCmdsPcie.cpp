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

    // Launch PCIE through IDevice Abstraction
    devLayer_ = dev::IDeviceLayer::createPcieDeviceLayer(true, false);

    initTestHelper();
  }
};

TEST_F(TestDevOpsApiNightlyBasicCmdsPcie, backToBack3kSameCmdsSingleDeviceSingleQueue_1_3) {
  backToBackSameCmdsSingleDeviceSingleQueue_1_1(3000);
}

TEST_F(TestDevOpsApiNightlyBasicCmdsPcie, backToBack3kSameCmdsSingleDeviceMultiQueue_1_4) {
  backToBackSameCmdsSingleDeviceMultiQueue_1_2(3000);
}

/* this test takes too long to run in Zebu - hence will only limit to 9K with multi queue
TEST_F(TestDevOpsApiNightlyBasicCmdsPcie, backToBack9kDiffCmdsSingleQueue_1_5) {
  backToBackDiffCmdsSingleQueue_1_3(9000);
}
*/

TEST_F(TestDevOpsApiNightlyBasicCmdsPcie, backToBack3kDiffCmdsSingleDeviceMultiQueue_1_6) {
  backToBackDiffCmdsSingleDeviceMultiQueue_1_4(3000);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
