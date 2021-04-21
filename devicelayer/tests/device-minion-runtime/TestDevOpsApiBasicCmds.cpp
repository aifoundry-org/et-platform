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
#include "Autogen.h"

/***********************************************************
 *                                                         *
 *                   Functional Tests                      *
 *                                                         *
 **********************************************************/
void TestDevOpsApiBasicCmds::echoCmd_PositiveTest_2_1() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  uint8_t queueCount = devLayer_->getSubmissionQueuesCount(kIDevice);
  initTagId(0x01);

  for (int queueId = 0; queueId < queueCount; queueId++) {
    // Add cmd to stream
    stream.push_back(IDevOpsApiCmd::createEchoCmd(getNextTagId(), false, kEchoPayload));

    // Move stream to streams_
    streams_.emplace(queueId, std::move(stream));
  }

  executeAsync();
}

void TestDevOpsApiBasicCmds::devCompatCmd_PositiveTest_2_3() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  uint8_t queueCount = devLayer_->getSubmissionQueuesCount(kIDevice);
  initTagId(0x11);

  for (int queueId = 0; queueId < queueCount; queueId++) {
    // Add cmd to stream
    stream.push_back(
      IDevOpsApiCmd::createApiCompatibilityCmd(getNextTagId(), false, kDevFWMajor, kDevFWMinor, kDevFWPatch));

    // Move stream to streams_
    streams_.emplace(queueId, std::move(stream));
  }

  executeAsync();
}

void TestDevOpsApiBasicCmds::devFWCmd_PostiveTest_2_5() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  uint8_t queueCount = devLayer_->getSubmissionQueuesCount(kIDevice);
  initTagId(0x21);

  for (int queueId = 0; queueId < queueCount; queueId++) {
    // Add cmd to stream
    stream.push_back(IDevOpsApiCmd::createFwVersionCmd(getNextTagId(), false, 1));

    // Move stream to streams_
    streams_.emplace(queueId, std::move(stream));
  }

  executeAsync();
}

void TestDevOpsApiBasicCmds::devUnknownCmd_NegativeTest_2_7() {
  initTagId(0x31);

  device_ops_api::cmd_header_t unknownCmd;
  unknownCmd.cmd_hdr.size = sizeof(unknownCmd);
  unknownCmd.cmd_hdr.tag_id = getNextTagId();
  unknownCmd.cmd_hdr.msg_id = 0xdead; // Unknown message Id
  unknownCmd.flags = 0;               // No barrier

  // Create unknown command
  auto cmd = IDevOpsApiCmd::createCustomCmd(reinterpret_cast<std::byte*>(&unknownCmd), sizeof(unknownCmd), 0);

  // TODO SW-6818: Use executeAsync()/executeSync() instead when waitForEpollEventsMasterMinion()
  // is functional with timeout in sysemu
  uint8_t queueId = 0;
  auto res = pushCmd(queueId, cmd);
  ASSERT_TRUE(res) << "Unable to send the unknown command!";

  TEST_VLOG(1) << "Waiting for some time to see if response is received for unknown command ...";
  std::this_thread::sleep_for(std::chrono::seconds(5));

  res = popRsp();
  ASSERT_FALSE(res) << "ERROR: Response received for unknown command!";
  TEST_VLOG(1) << "No response received for unknown command as expected";

  deleteCmdResults();
}

/***********************************************************
 *                                                         *
 *                      Stress Tests                       *
 *                                                         *
 **********************************************************/
void TestDevOpsApiBasicCmds::backToBackSameCmds_1_1(int numOfIterations) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  uint8_t queueCount = devLayer_->getSubmissionQueuesCount(kIDevice);
  initTagId(0x31);

  for (int queueId = 0; queueId < queueCount; queueId++) {
    for (int i = 0; i < numOfIterations; i++) {
      // Add cmd to stream
      stream.push_back(IDevOpsApiCmd::createEchoCmd(getNextTagId(), false, kEchoPayload));
    }

    // Move stream to streams_
    streams_.emplace(queueId, std::move(stream));
  }

  executeAsync();
}

void TestDevOpsApiBasicCmds::backToBackDiffCmds_1_2(int numOfIterations) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  uint8_t queueCount = devLayer_->getSubmissionQueuesCount(kIDevice);
  initTagId(0x41);

  for (int i = 0; i < numOfIterations; i++) {
    // Add cmd to stream
    stream.push_back(IDevOpsApiCmd::createEchoCmd(getNextTagId(), false, kEchoPayload));
    stream.push_back(IDevOpsApiCmd::createFwVersionCmd(getNextTagId(), false, 1));
    stream.push_back(
      IDevOpsApiCmd::createApiCompatibilityCmd(getNextTagId(), false, kDevFWMajor, kDevFWMinor, kDevFWPatch));
  }

  // Move stream to streams_
  streams_.emplace(0, std::move(stream));

  executeAsync();
}
