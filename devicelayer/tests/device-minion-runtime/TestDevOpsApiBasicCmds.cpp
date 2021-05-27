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
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    int queueCount = getSqCount(deviceIdx);

    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      // Add cmd to stream
      stream.push_back(IDevOpsApiCmd::createEchoCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kEchoPayload));

      // Move stream to streams_
      streams_.emplace(key(deviceIdx, queueIdx), std::move(stream));
    }
  }

  executeAsync();
}

void TestDevOpsApiBasicCmds::devCompatCmd_PositiveTest_2_3() {
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    uint8_t queueCount = getSqCount(deviceIdx);

    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      // Add cmd to stream
      stream.push_back(IDevOpsApiCmd::createApiCompatibilityCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kDevFWMajor,
                                                                kDevFWMinor, kDevFWPatch));

      // Move stream to streams_
      streams_.emplace(key(deviceIdx, queueIdx), std::move(stream));
    }
  }
  executeAsync();
}

void TestDevOpsApiBasicCmds::devFWCmd_PostiveTest_2_5() {
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    uint8_t queueCount = getSqCount(deviceIdx);

    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      // Add cmd to stream
      stream.push_back(IDevOpsApiCmd::createFwVersionCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, 1));

      // Move stream to streams_
      streams_.emplace(key(deviceIdx, queueIdx), std::move(stream));
    }
  }
  executeAsync();
}

void TestDevOpsApiBasicCmds::devUnknownCmd_NegativeTest_2_7() {
  int deviceIdx = 0;
  device_ops_api::cmd_header_t unknownCmd;
  unknownCmd.cmd_hdr.size = sizeof(unknownCmd);
  unknownCmd.cmd_hdr.tag_id = 0x1;
  unknownCmd.cmd_hdr.msg_id = 0xdead; // Unknown message Id
  unknownCmd.cmd_hdr.flags = 0;       // No barrier

  // Create unknown command
  auto cmd = IDevOpsApiCmd::createCustomCmd(reinterpret_cast<std::byte*>(&unknownCmd), sizeof(unknownCmd), 0);

  // TODO SW-6818: Use executeAsync()/executeSync() instead when waitForEpollEventsMasterMinion()
  // is functional with timeout in sysemu
  uint8_t queueCount = getSqCount(deviceIdx);

  for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
    // Push command to all the availabel queus one by one.
    auto res = pushCmd(deviceIdx, queueIdx, cmd);
    ASSERT_TRUE(res) << "Unable to send the unknown command!";
  }

  TEST_VLOG(1) << "Waiting for some time to see if response is received for unknown command ...";
  std::this_thread::sleep_for(std::chrono::seconds(5));

  auto res = popRsp(deviceIdx);
  ASSERT_FALSE(res) << "ERROR: Response received for unknown command!";
  TEST_VLOG(1) << "No response received for unknown command as expected";

  deleteCmdResults();
}

/***********************************************************
 *                                                         *
 *                      Stress Tests                       *
 *                                                         *
 **********************************************************/
void TestDevOpsApiBasicCmds::backToBackSameCmdsSingleDeviceSingleQueue_1_1(int numOfCmds) {
  int deviceCount = 1;
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    int queueIdx = 0;
    for (int i = 0; i < numOfCmds; i++) {
      // Add cmd to stream
      stream.push_back(IDevOpsApiCmd::createEchoCmd(false, kEchoPayload));
    }
    // Move stream to streams_
    streams_.emplace(key(deviceIdx, queueIdx), std::move(stream));
  }
  executeAsync();
}

void TestDevOpsApiBasicCmds::backToBackSameCmdsSingleDeviceMultiQueue_1_2(int numOfCmds) {
  int deviceCount = 1;
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    uint8_t queueCount = getSqCount(deviceIdx);

    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      for (int i = 0; i < (numOfCmds / queueCount); i++) {
        // Add cmd to stream
        stream.push_back(IDevOpsApiCmd::createEchoCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kEchoPayload));
      }

      // Move stream to streams_
      streams_.emplace(key(deviceIdx, queueIdx), std::move(stream));
    }
  }
  executeAsync();
}

void TestDevOpsApiBasicCmds::backToBackDiffCmdsSingleDeviceSingleQueue_1_3(int numOfCmds) {
  int deviceCount = 1;
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    int queueIdx = 0;

    for (int i = 0; i < numOfCmds; i++) {
      // Add cmd to stream
      stream.push_back(IDevOpsApiCmd::createEchoCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kEchoPayload));
      stream.push_back(IDevOpsApiCmd::createFwVersionCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, 1));
      stream.push_back(IDevOpsApiCmd::createApiCompatibilityCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kDevFWMajor,
                                                                kDevFWMinor, kDevFWPatch));
    }
    // Move stream to streams_
    streams_.emplace(key(deviceIdx, queueIdx), std::move(stream));
  }
  executeAsync();
}

void TestDevOpsApiBasicCmds::backToBackDiffCmdsSingleDeviceMultiQueue_1_4(int numOfCmds) {
  int deviceCount = 1;
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    uint8_t queueCount = getSqCount(deviceIdx);

    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      for (int i = 0; i < (numOfCmds / queueCount); i++) {
        // Add cmd to stream
        stream.push_back(IDevOpsApiCmd::createEchoCmd(false, kEchoPayload));
        stream.push_back(IDevOpsApiCmd::createFwVersionCmd(false, 1));
        stream.push_back(IDevOpsApiCmd::createApiCompatibilityCmd(false, kDevFWMajor, kDevFWMinor, kDevFWPatch));
      }
      // Move stream to streams_
      streams_.emplace(key(deviceIdx, queueIdx), std::move(stream));
    }
  }
  executeAsync();
}

void TestDevOpsApiBasicCmds::backToBackSameCmdsMultiDeviceMultiQueue_1_5(int numOfCmds) {
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    uint8_t queueCount = getSqCount(deviceIdx);

    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      for (int i = 0; i < ((numOfCmds / deviceCount) / queueCount); i++) {
        // Add cmd to stream
        stream.push_back(IDevOpsApiCmd::createEchoCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kEchoPayload));
      }

      // Move stream to streams_
      streams_.emplace(key(deviceIdx, queueIdx), std::move(stream));
    }
  }
  executeAsync();
}

void TestDevOpsApiBasicCmds::backToBackDiffCmdsMultiDeviceMultiQueue_1_6(int numOfCmds) {
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    uint8_t queueCount = getSqCount(deviceIdx);

    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      for (int i = 0; i < ((numOfCmds / deviceCount) / queueCount); i++) {
        // Add cmd to stream
        stream.push_back(IDevOpsApiCmd::createEchoCmd(false, kEchoPayload));
        stream.push_back(IDevOpsApiCmd::createFwVersionCmd(false, 1));
        stream.push_back(IDevOpsApiCmd::createApiCompatibilityCmd(false, kDevFWMajor, kDevFWMinor, kDevFWPatch));
      }
      // Move stream to streams_
      streams_.emplace(key(deviceIdx, queueIdx), std::move(stream));
    }
  }
  executeAsync();
}
