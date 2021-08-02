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

using namespace dev::dl_tests;

class TestDevOpsApiLoopbackBasicCmdsPcie : public TestDevOpsApiBasicCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);

    initTestHelperPcie();
  }
};

TEST_F(TestDevOpsApiLoopbackBasicCmdsPcie, devUnknownCmd_1_0) {
  devUnknownCmd_NegativeTest_2_7();
}

TEST_F(TestDevOpsApiLoopbackBasicCmdsPcie, backToBackSame50kCmdsSingleDeviceSingleQueue_1_1) {
  backToBackSameCmds(true /* single device */, true /* single queue */, 50000);
}

TEST_F(TestDevOpsApiLoopbackBasicCmdsPcie, backToBackSame50kCmdsSingleDeviceMultiQueue_1_2) {
  backToBackSameCmds(true /* single device */, false /* multiple queues */, 50000);
}

TEST_F(TestDevOpsApiLoopbackBasicCmdsPcie, backToBackSame50kCmdsMultiDeviceMultiQueue_1_3) {
  backToBackSameCmds(false /* multiple devices */, false /* multiple queues */, 50000);
}

TEST_F(TestDevOpsApiLoopbackBasicCmdsPcie, backToBackDiff50kCmdsSingleDeviceSingleQueue_1_4) {
  backToBackDiffCmds(true /* single device */, true /* single queue */, 50000);
}

TEST_F(TestDevOpsApiLoopbackBasicCmdsPcie, backToBackDiff50kCmdsSingleDeviceMultiQueue_1_5) {
  backToBackDiffCmds(true /* single device */, false /* multiple queues */, 50000);
}

TEST_F(TestDevOpsApiLoopbackBasicCmdsPcie, backToBackDiff50kCmdsMultiDeviceMultiQueue_1_6) {
  backToBackDiffCmds(false /* multiple devices */, false /* multiple queues */, 50000);
}

TEST_F(TestDevOpsApiLoopbackBasicCmdsPcie, backToBackDiff50kCmdsSingleDeviceMultiQueueWithoutEpoll_1_7) {
  FLAGS_use_epoll = false;
  backToBackDiffCmds(false /* multiple devices */, false /* multiple queues */, 50000);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  if (FLAGS_loopback_driver) {
    // Loopback driver does not support trace
    FLAGS_enable_trace_dump = false;
  }
  return RUN_ALL_TESTS();
}
