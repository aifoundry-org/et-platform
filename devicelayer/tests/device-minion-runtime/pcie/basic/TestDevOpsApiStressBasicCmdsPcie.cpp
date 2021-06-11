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

class TestDevOpsApiStressBasicCmdsPcie : public TestDevOpsApiBasicCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);

    initTestHelperPcie();
  }
};

TEST_F(TestDevOpsApiStressBasicCmdsPcie, backToBackSame1kCmdsSingleDeviceSingleQueue_1_1) {
  backToBackSameCmds(true /* single device */, true /* single queue */, 1000);
}

TEST_F(TestDevOpsApiStressBasicCmdsPcie, backToBackSame1kCmdsSingleDeviceMultiQueue_1_2) {
  backToBackSameCmds(true /* single device */, false /* multiple queues */, 1000);
}

TEST_F(TestDevOpsApiStressBasicCmdsPcie, backToBackDiff1kCmdsSingleDeviceSingleQueue_1_3) {
  backToBackDiffCmds(true /* single device */, true /* single queue */, 1000);
}

TEST_F(TestDevOpsApiStressBasicCmdsPcie, backToBackDiff1kCmdsSingleDeviceMultiQueue_1_4) {
  backToBackDiffCmds(true /* single device */, false /* multiple queues */, 1000);
}

TEST_F(TestDevOpsApiStressBasicCmdsPcie, backToBackDiff1kCmdsSingleDeviceMultiQueueWithoutEpoll_1_5) {
  FLAGS_use_epoll = false;
  backToBackDiffCmds(true /* single device */, false /* multiple queues */, 1000);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
