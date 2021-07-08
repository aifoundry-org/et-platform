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

using namespace dev::dl_tests;

/***********************************************************
 *                                                         *
 *                   Functional Tests                      *
 *                                                         *
 **********************************************************/
void TestDevOpsApiBasicCmds::echoCmd_PositiveTest_2_1() {
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<CmdTag> stream;
    int queueCount = getSqCount(deviceIdx);

    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      // Add cmd to stream
      stream.push_back(IDevOpsApiCmd::createCmd<EchoCmd>(device_ops_api::CMD_FLAGS_BARRIER_DISABLE));

      // Save stream against deviceIdx and queueIdx
      insertStream(deviceIdx, queueIdx, std::move(stream));
      stream.clear();
    }
  }

  execute(true);
  deleteStreams();
}

void TestDevOpsApiBasicCmds::devCompatCmd_PositiveTest_2_3() {
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<CmdTag> stream;
    uint8_t queueCount = getSqCount(deviceIdx);

    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      // Add cmd to stream
      stream.push_back(IDevOpsApiCmd::createCmd<ApiCompatibilityCmd>(device_ops_api::CMD_FLAGS_BARRIER_DISABLE,
                                                                     kDevFWMajor, kDevFWMinor, kDevFWPatch));
      // Save stream against deviceIdx and queueIdx
      insertStream(deviceIdx, queueIdx, std::move(stream));
      stream.clear();
    }
  }

  execute(true);
  deleteStreams();
}

void TestDevOpsApiBasicCmds::devFWCmd_PostiveTest_2_5() {
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<CmdTag> stream;
    uint8_t queueCount = getSqCount(deviceIdx);

    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      // Add cmd to stream
      stream.push_back(IDevOpsApiCmd::createCmd<FwVersionCmd>(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, 1));

      // Save stream against deviceIdx and queueIdx
      insertStream(deviceIdx, queueIdx, std::move(stream));
      stream.clear();
    }
  }
  execute(true);
  deleteStreams();
}

void TestDevOpsApiBasicCmds::devUnknownCmd_NegativeTest_2_7() {
  device_ops_api::cmd_header_t unknownCmd;
  unknownCmd.cmd_hdr.size = sizeof(unknownCmd);
  unknownCmd.cmd_hdr.tag_id = 0x1;
  unknownCmd.cmd_hdr.msg_id = 0xdead; // Unknown message Id
  unknownCmd.cmd_hdr.flags = 0;       // No barrier

  // Create unknown command
  auto tagId = IDevOpsApiCmd::createCmd<CustomCmd>(templ::bit_cast<std::byte*>(&unknownCmd), sizeof(unknownCmd));

  int deviceIdx = 0;
  controlTraceLogging(deviceIdx, true /* to trace buffer */, true /* Reset trace buffer. */);

  auto queueCount = getSqCount(deviceIdx);
  for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
    // Push command to all the availabel queus one by one.
    auto res = pushCmd(deviceIdx, queueIdx, tagId);
    EXPECT_TRUE(res) << "Unable to send the unknown command!";
  }

  TEST_VLOG(1) << "Waiting for some time to see if response is received for unknown command ...";
  std::this_thread::sleep_for(std::chrono::seconds(5));

  EXPECT_FALSE(popRsp(deviceIdx)) << "ERROR: Response received for unknown command!";

  controlTraceLogging(deviceIdx, false /* to UART */, false /* don't reset Trace buffer*/);
}

/***********************************************************
 *                                                         *
 *                      Stress Tests                       *
 *                                                         *
 **********************************************************/
void TestDevOpsApiBasicCmds::backToBackSameCmds(bool singleDevice, bool singleQueue, int numOfCmds) {
  int deviceCount = (singleDevice) ? 1 : getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<CmdTag> stream;

    auto queueCount = (singleQueue) ? 1 : getSqCount(deviceIdx);
    auto perDevPerQueueIterations = numOfCmds / deviceCount / queueCount;
    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      for (int i = 0; i < perDevPerQueueIterations; i++) {
        // Add cmd to stream
        stream.push_back(IDevOpsApiCmd::createCmd<EchoCmd>(device_ops_api::CMD_FLAGS_BARRIER_DISABLE));
      }

      // Save stream against deviceIdx and queueIdx
      insertStream(deviceIdx, queueIdx, std::move(stream));
      stream.clear();
    }
  }
  execute(true);
  deleteStreams();
}

void TestDevOpsApiBasicCmds::backToBackDiffCmds(bool singleDevice, bool singleQueue, int numOfCmds) {
  int deviceCount = (singleDevice) ? 1 : getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<CmdTag> stream;

    auto queueCount = (singleQueue) ? 1 : getSqCount(deviceIdx);
    auto perDevPerQueueIterations = numOfCmds / 3 / deviceCount / queueCount;
    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      for (int i = 0; i < perDevPerQueueIterations; i++) {
        // Add cmd to stream
        stream.push_back(IDevOpsApiCmd::createCmd<EchoCmd>(false));
        stream.push_back(IDevOpsApiCmd::createCmd<FwVersionCmd>(false, 1));
        stream.push_back(IDevOpsApiCmd::createCmd<ApiCompatibilityCmd>(false, kDevFWMajor, kDevFWMinor, kDevFWPatch));
      }
      // Save stream against deviceIdx and queueIdx
      insertStream(deviceIdx, queueIdx, std::move(stream));
      stream.clear();
    }
  }
  execute(true);
  deleteStreams();
}
