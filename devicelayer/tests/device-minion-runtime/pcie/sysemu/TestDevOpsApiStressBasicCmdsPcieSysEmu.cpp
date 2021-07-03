//******************************************************************************
// Copyright (C) 2021 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestDevOpsApiBasicCmds.h"

/*
 * Test Labels: PCIE-SYSEMU, OPS, FUNCTIONAL, SYSTEM
 */

using namespace dev::dl_tests;

class TestDevOpsApiStressBasicCmdsPcieSysEmu : public TestDevOpsApiBasicCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);
    initTestHelperPcie();
  }

};

TEST_F(TestDevOpsApiStressBasicCmdsPcieSysEmu, backToBackSame1kCmdsSingleDeviceSingleQueue_1_1) {
  backToBackSameCmds(true /* single device */, true /* single queue */, 1000);
}

TEST_F(TestDevOpsApiStressBasicCmdsPcieSysEmu, backToBackSame1kCmdsSingleDeviceMultiQueue_1_2) {
  backToBackSameCmds(true /* single device */, false /* multiple queues */, 1000);
}

TEST_F(TestDevOpsApiStressBasicCmdsPcieSysEmu, backToBackDiff1kCmdsSingleDeviceSingleQueue_1_3) {
  backToBackDiffCmds(true /* single device */, true /* single queue */, 1000);
}

TEST_F(TestDevOpsApiStressBasicCmdsPcieSysEmu, backToBackDiff1kCmdsSingleDeviceMultiQueue_1_4) {
  backToBackDiffCmds(true /* single device */, false /* multiple queues */, 1000);
}

TEST_F(TestDevOpsApiStressBasicCmdsPcieSysEmu, backToBackSame10kCmdsSingleDeviceSingleQueue_1_5) {
  backToBackSameCmds(true /* single device */, true /* single queue */, 10000);
}

TEST_F(TestDevOpsApiStressBasicCmdsPcieSysEmu, backToBackSame10kCmdsSingleDeviceMultiQueue_1_6) {
  backToBackSameCmds(true /* single device */, false /* multiple queues */, 10000);
}

TEST_F(TestDevOpsApiStressBasicCmdsPcieSysEmu, backToBackDiff10kCmdsSingleDeviceSingleQueue_1_7) {
  backToBackDiffCmds(true /* single device */, true /* single queue */, 10000);
}

TEST_F(TestDevOpsApiStressBasicCmdsPcieSysEmu, backToBackDiff10kCmdsSingleDeviceMultiQueue_1_8) {
  backToBackDiffCmds(true /* single device */, false /* multiple queues */, 10000);
}

TEST_F(TestDevOpsApiStressBasicCmdsPcieSysEmu, backToBackDiff10kCmdsSingleDeviceMultiQueueWithoutEpoll_1_9) {
  FLAGS_use_epoll = false;
  backToBackDiffCmds(true /* single device */, false /* multiple queues */, 10000);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
