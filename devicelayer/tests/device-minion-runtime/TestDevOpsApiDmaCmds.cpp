//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestDevOpsApiDmaCmds.h"
#include "Autogen.h"
#include <fcntl.h>

/**********************************************************
 *                                                         *
 *              DMA Basic Testing Functions                *
 *                                                         *
 **********************************************************/
void TestDevOpsApiDmaCmds::dataRWCmdWithBasicCmds_3_4() {
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  int deviceCount = getDevicesCount();

  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    uint8_t queueCount = getSqCount(deviceIdx);
    const uint64_t bufSize = 64;
    std::vector<uint8_t> dmaWrBuf;
    std::vector<uint8_t> dmaRdBuf;

    for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
      dmaWrBuf.resize(bufSize, 0);
      for (int i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = rand() % 0x100;
      }
      stream.push_back(IDevOpsApiCmd::createEchoCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kEchoPayload));
      stream.push_back(IDevOpsApiCmd::createApiCompatibilityCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kDevFWMajor,
                                                                kDevFWMinor, kDevFWPatch));
      stream.push_back(IDevOpsApiCmd::createFwVersionCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, 1));

      auto devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
      auto hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                         hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaWrBufs.push_back(std::move(dmaWrBuf));
      dmaRdBuf.resize(bufSize, 0);
      devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devPhysAddr,
                                                        hostVirtAddr, hostPhysAddr, dmaRdBuf.size(),
                                                        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

      dmaRdBufs.push_back(std::move(dmaRdBuf));
      // Move stream of commands to streams_
      streams_.emplace(key(deviceIdx, queueId), std::move(stream));
    }
  }
  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // For DMA received Data Validation
  for (size_t i = 0; i < dmaWrBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
  }
}

// Read/Write access mixed b2b .. i.e. DMA WRITE, DMA READ,
// DMA READ, DMA WRITE. The address would be orthogonal to each other.
void TestDevOpsApiDmaCmds::dataRWCmdMixed_3_5() {
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    uint8_t queueCount = getSqCount(deviceIdx);
    const int buffSizeA = 512;
    const int buffSizeB = 256;
    // Create some buffers for send and receiving data
    std::vector<uint8_t> dmaWrBuf;
    std::vector<uint8_t> dmaRdBuf;

    for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
      dmaWrBuf.resize(buffSizeA, 0);
      for (int i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = rand() % 0x100;
      }
      auto devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
      auto hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                         hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaWrBufs.push_back(std::move(dmaWrBuf));

      dmaWrBuf.resize(buffSizeA);
      for (int i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = rand() % 0x100;
      }
      devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                         hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaWrBufs.push_back(std::move(dmaWrBuf));

      dmaRdBuf.resize(buffSizeA, 0);
      devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devPhysAddr,
                                                        hostVirtAddr, hostPhysAddr, dmaRdBuf.size(),
                                                        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaRdBufs.push_back(std::move(dmaRdBuf));

      dmaRdBuf.resize(buffSizeA, 0);
      devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                        hostVirtAddr, hostPhysAddr, dmaRdBuf.size(),
                                                        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaRdBufs.push_back(std::move(dmaRdBuf));

      dmaWrBuf.resize(buffSizeB, 0);
      for (int i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = rand() % 0x100;
      }
      devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                         hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaWrBufs.push_back(std::move(dmaWrBuf));

      dmaRdBuf.resize(buffSizeB, 0);
      devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devPhysAddr,
                                                        hostVirtAddr, hostPhysAddr, dmaRdBuf.size(),
                                                        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaRdBufs.push_back(std::move(dmaRdBuf));

      // Move stream of commands to streams_
      streams_.emplace(key(deviceIdx, queueId), std::move(stream));
    }
  }
  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // For DMA received Data Validation
  for (std::size_t i = 0; i < dmaWrBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
  }
}

// Add variable sized to each transaction, you can pick from
// an array of 5-10 different sizes randomly, and assign to
// the different channels different sizes
void TestDevOpsApiDmaCmds::dataRWCmdMixedWithVarSize_3_6() {
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    uint8_t queueCount = getSqCount(deviceIdx);

    static const std::array<uint64_t, 10> bufSizeArray = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    srand(time(0));
    uint64_t bufSize[3];
    bufSize[0] = bufSizeArray[rand() % bufSizeArray.size()];
    bufSize[1] = bufSizeArray[rand() % bufSizeArray.size()];
    bufSize[2] = bufSizeArray[rand() % bufSizeArray.size()];
    TEST_VLOG(1) << "rwBuffer Values are " << bufSize[0] << ", " << bufSize[1] << ", " << bufSize[2] << std::endl;

    for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {

      std::vector<uint8_t> dmaWrBuf(bufSize[0], 0);
      std::vector<uint8_t> dmaRdBuf(bufSize[0], 0);
      // Data Write Cmd1 with bufSize[0]
      for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = rand() % 0x100;
      }
      auto devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
      auto hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                         hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaWrBufs.push_back(std::move(dmaWrBuf));

      // Data Write Cmd2 with bufSize[1]
      dmaWrBuf.resize(bufSize[1], 0);
      for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = rand() % 0x100;
      }
      devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                         hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaWrBufs.push_back(std::move(dmaWrBuf));

      // Data Read Cmd1 with bufSize[0]
      devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devPhysAddr,
                                                        hostVirtAddr, hostPhysAddr, dmaRdBuf.size(),
                                                        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaRdBufs.push_back(std::move(dmaRdBuf));

      // Data Read Cmd2 with bufSize[1]
      dmaRdBuf.resize(bufSize[1], 0);
      devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                        hostVirtAddr, hostPhysAddr, dmaRdBuf.size(),
                                                        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaRdBufs.push_back(std::move(dmaRdBuf));

      // Data Write Cmd3 with bufSize[2]
      dmaWrBuf.resize(bufSize[2], 0);
      for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = rand() % 0x100;
      }
      devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                         hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaWrBufs.push_back(std::move(dmaWrBuf));

      // Data Read Cmd3 with bufSize[2]
      dmaRdBuf.resize(bufSize[2], 0);
      devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devPhysAddr,
                                                        hostVirtAddr, hostPhysAddr, dmaRdBuf.size(),
                                                        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaRdBufs.push_back(std::move(dmaRdBuf));

      // Move stream of commands to streams_
      streams_.emplace(key(deviceIdx, queueId), std::move(stream));
    }
  }
  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // For DMA received Data Validation
  for (std::size_t i = 0; i < dmaWrBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
  }
}

// Add minimal 8 DMA transactions with each channel
// having different transfer size, with at least
// 1 CMD in the middle having a Barrier bit set
void TestDevOpsApiDmaCmds::dataRWCmdAllChannels_3_7() {
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  const uint64_t bufSize[4] = {256, 512, 1024, 2048};
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    uint8_t queueCount = getSqCount(deviceIdx);

    for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
      std::vector<uint8_t> dmaWrBuf(bufSize[0], 0);
      std::vector<uint8_t> dmaRdBuf(bufSize[0], 0);
      // Data Write Cmd1 with bufSize[0]
      for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = rand() % 0x100;
      }
      auto devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
      auto hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                         hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaWrBufs.push_back(std::move(dmaWrBuf));

      // Data Write Cmd2 with bufSize[1]
      dmaWrBuf.resize(bufSize[1], 0);
      for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = rand() % 0x100;
      }
      devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                         hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaWrBufs.push_back(std::move(dmaWrBuf));

      // Data Write Cmd3 with bufSize[2]
      dmaWrBuf.resize(bufSize[2], 0);
      for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = rand() % 0x100;
      }
      devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                         hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaWrBufs.push_back(std::move(dmaWrBuf));

      // Data Write Cmd4 with bufSize[3]
      dmaWrBuf.resize(bufSize[3], 0);
      for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = rand() % 0x100;
      }
      devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                         hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaWrBufs.push_back(std::move(dmaWrBuf));

      // Data Read Cmd1 with bufSize[0] with barrier
      devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devPhysAddr,
                                                        hostVirtAddr, hostPhysAddr, dmaRdBuf.size(),
                                                        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaRdBufs.push_back(std::move(dmaRdBuf));

      // Data Read Cmd2 with bufSize[1]
      dmaRdBuf.resize(bufSize[1], 0);
      devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                        hostVirtAddr, hostPhysAddr, dmaRdBuf.size(),
                                                        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaRdBufs.push_back(std::move(dmaRdBuf));

      // Data Read Cmd3 with bufSize[2]
      dmaRdBuf.resize(bufSize[2], 0);
      devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devPhysAddr,
                                                        hostVirtAddr, hostPhysAddr, dmaRdBuf.size(),
                                                        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaRdBufs.push_back(std::move(dmaRdBuf));

      // Data Read Cmd4 with bufSize[3]
      dmaRdBuf.resize(bufSize[3], 0);
      devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devPhysAddr,
                                                        hostVirtAddr, hostPhysAddr, dmaRdBuf.size(),
                                                        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaRdBufs.push_back(std::move(dmaRdBuf));

      // Move stream of commands to streams_[queueId]
      streams_.emplace(key(deviceIdx, queueId), std::move(stream));
    }
  }
  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // For DMA received Data Validation
  for (std::size_t i = 0; i < dmaWrBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
  }
}

/**********************************************************
 *                                                         *
 *             DMA Positive Testing Functions              *
 *                                                         *
 **********************************************************/
void TestDevOpsApiDmaCmds::dataRWCmd_PositiveTesting_3_1() {
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    uint8_t queueCount = getSqCount(deviceIdx);

    std::vector<uint8_t> dmaRdBuf;
    std::vector<uint8_t> dmaWrBuf;
    const uint64_t bufSize = 64;

    for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
      dmaWrBuf.resize(bufSize, 0);
      for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = static_cast<uint8_t>(rand() % 0x100);
      }
      auto devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
      auto hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                         hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaWrBufs.push_back(std::move(dmaWrBuf));

      // Read data
      dmaRdBuf.resize(bufSize, 0);
      devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devPhysAddr,
                                                        hostVirtAddr, hostPhysAddr, dmaRdBuf.size(),
                                                        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaRdBufs.push_back(std::move(dmaRdBuf));

      // Move stream of commands to streams_[queueId]
      streams_.emplace(key(deviceIdx, queueId), std::move(stream));
    }
  }
  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // Validate data received from DMA
  for (std::size_t i = 0; i < dmaWrBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataRWCmdWithBarrier_PositiveTesting_3_10() {
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    uint8_t queueCount = getSqCount(deviceIdx);
    const uint64_t dmaWrBufsize = 512;
    const uint64_t dmaRdBufsize = 4 * dmaWrBufsize;
    std::vector<uint8_t> dmaWrBuf;
    std::vector<uint8_t> dmaRdBuf;

    for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
      dmaWrBuf.resize(dmaWrBufsize, 0);
      for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = rand() % 0x100;
      }
      auto devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
      auto hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                         hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaWrBufs.push_back(std::move(dmaWrBuf));

      dmaWrBuf.resize(dmaWrBufsize, 0);
      for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = rand() % 0x100;
      }
      devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                         hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaWrBufs.push_back(std::move(dmaWrBuf));

      dmaWrBuf.resize(dmaWrBufsize, 0);
      for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = rand() % 0x100;
      }
      devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                         hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaWrBufs.push_back(std::move(dmaWrBuf));

      dmaWrBuf.resize(dmaWrBufsize, 0);
      for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = rand() % 0x100;
      }
      devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                         hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaWrBufs.push_back(std::move(dmaWrBuf));

      // since we sent 4 DMA write cmd, we need to read back all the data in a single DMA read cmd
      dmaRdBuf.resize(dmaRdBufsize, 0);
      devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
      hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devPhysAddr,
                                                        hostVirtAddr, hostPhysAddr, dmaRdBuf.size(),
                                                        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaRdBufs.push_back(std::move(dmaRdBuf));

      // Move stream of commands to streams_
      streams_.emplace(key(deviceIdx, queueId), std::move(stream));
    }
  }
  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // For DMA received Data Validation
  for (uint8_t dmaRdBufIdx = 0; dmaRdBufIdx < dmaRdBufs.size(); dmaRdBufIdx++) {
    uint8_t writeIdxOffset = dmaRdBufIdx * 4;
    EXPECT_EQ(dmaWrBufs[writeIdxOffset],
              std::vector<uint8_t>(dmaRdBufs[dmaRdBufIdx].begin(),
                                   dmaRdBufs[dmaRdBufIdx].begin() + dmaWrBufs[writeIdxOffset].size()));
    EXPECT_EQ(dmaWrBufs[writeIdxOffset + 1],
              std::vector<uint8_t>(dmaRdBufs[dmaRdBufIdx].begin() + dmaWrBufs[writeIdxOffset].size(),
                                   dmaRdBufs[dmaRdBufIdx].begin() + dmaWrBufs[writeIdxOffset].size() +
                                     dmaWrBufs[writeIdxOffset + 1].size()));
    EXPECT_EQ(dmaWrBufs[writeIdxOffset + 2],
              std::vector<uint8_t>(dmaRdBufs[dmaRdBufIdx].begin() + dmaWrBufs[writeIdxOffset].size() +
                                     dmaWrBufs[writeIdxOffset + 1].size(),
                                   dmaRdBufs[dmaRdBufIdx].begin() + dmaWrBufs[writeIdxOffset].size() +
                                     dmaWrBufs[writeIdxOffset + 1].size() + dmaWrBufs[writeIdxOffset + 2].size()));
    EXPECT_EQ(dmaWrBufs[writeIdxOffset + 3],
              std::vector<uint8_t>(dmaRdBufs[dmaRdBufIdx].begin() + dmaWrBufs[writeIdxOffset].size() +
                                     dmaWrBufs[writeIdxOffset + 1].size() + dmaWrBufs[writeIdxOffset + 2].size(),
                                   dmaRdBufs[dmaRdBufIdx].end()));
  }
}

void TestDevOpsApiDmaCmds::dataRWListCmd_PositiveTesting_3_11() {
  std::vector<void*> dmaWrMemPtrs;
  std::vector<void*> dmaRdMemPtrs;
  const uint8_t kNodeCount = 4;
  const size_t kBufSize = 64;
  device_ops_api::dma_write_node wrList[kNodeCount];
  device_ops_api::dma_read_node rdList[kNodeCount];

  auto randFd = open("/dev/urandom", O_RDONLY);
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto queueCount = getSqCount(deviceIdx);
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;

    dmaWrMemPtrs.push_back(allocDmaBuffer(deviceIdx, queueCount * kNodeCount * kBufSize, true));
    dmaRdMemPtrs.push_back(allocDmaBuffer(deviceIdx, queueCount * kNodeCount * kBufSize, false));

    // Randomize write buffer
    read(randFd, dmaWrMemPtrs[deviceIdx], queueCount * kNodeCount * kBufSize);

    uint8_t* rdBufPtr = static_cast<uint8_t*>(dmaRdMemPtrs[deviceIdx]);
    uint8_t* wrBufPtr = static_cast<uint8_t*>(dmaWrMemPtrs[deviceIdx]);

    for (int queueIdx = 0; queueIdx < queueCount; ++queueIdx) {
      for (int nodeIdx = 0; nodeIdx < kNodeCount; ++nodeIdx) {
        // NOTE: host_phys_addr should be handled in SysEmu, userspace should not fill this value
        wrList[nodeIdx] = {.src_host_virt_addr = templ::bit_cast<uint64_t>(wrBufPtr),
                           .src_host_phy_addr = templ::bit_cast<uint64_t>(wrBufPtr),
                           .dst_device_phy_addr = getDmaWriteAddr(deviceIdx, kBufSize),
                           .size = kBufSize};
        wrBufPtr += kBufSize;

        rdList[nodeIdx] = {.dst_host_virt_addr = templ::bit_cast<uint64_t>(rdBufPtr),
                           .dst_host_phy_addr = templ::bit_cast<uint64_t>(rdBufPtr),
                           .src_device_phy_addr = getDmaReadAddr(deviceIdx, kBufSize),
                           .size = kBufSize};
        rdBufPtr += kBufSize;
      }
      stream.push_back(IDevOpsApiCmd::createDmaWriteListCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, wrList,
                                                            kNodeCount,
                                                            device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      stream.push_back(IDevOpsApiCmd::createDmaReadListCmd(device_ops_api::CMD_FLAGS_BARRIER_ENABLE, rdList, kNodeCount,
                                                           device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      // Move stream of commands to streams_[queueId]
      streams_.emplace(key(deviceIdx, queueIdx), std::move(stream));
    }
  }
  close(randFd);

  executeAsync();

  // Validate data received from DMA
  for (int deviceIdx = 0; deviceIdx < deviceCount; ++deviceIdx) {
    // Skip data validation in case of loopback driver
    if (!FLAGS_loopback_driver) {
      auto queueCount = getSqCount(deviceIdx);
      auto bytesWritten = queueCount * kNodeCount * kBufSize;
      EXPECT_EQ(memcmp(dmaWrMemPtrs[deviceIdx], dmaRdMemPtrs[deviceIdx], bytesWritten), 0)
        << "DMA W/R data mismatched for deviceIdx: " << (int)deviceIdx << std::endl;
    }
    freeDmaBuffer(dmaWrMemPtrs[deviceIdx]);
    freeDmaBuffer(dmaRdMemPtrs[deviceIdx]);
  }
}

/**********************************************************
 *                                                         *
 *             DMA Negative Testing Functions              *
 *                                                         *
 **********************************************************/
void TestDevOpsApiDmaCmds::dataRWListCmd_NegativeTesting_3_12() {
  std::vector<void*> dmaWrMemPtrs;
  std::vector<void*> dmaRdMemPtrs;
  const uint8_t kNodeCount = 4;
  const size_t kBufSize = 64;
  device_ops_api::dma_write_node wrList[kNodeCount];
  device_ops_api::dma_read_node rdList[kNodeCount];

  auto randFd = open("/dev/urandom", O_RDONLY);
  int deviceCount = getDevicesCount();
  int queueIdx = 0;
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;

    dmaWrMemPtrs.push_back(allocDmaBuffer(deviceIdx, kNodeCount * kBufSize, true));
    dmaRdMemPtrs.push_back(allocDmaBuffer(deviceIdx, kNodeCount * kBufSize, false));

    // Randomize write buffer
    read(randFd, dmaWrMemPtrs[deviceIdx], kNodeCount * kBufSize);

    uint8_t* rdBufPtr = static_cast<uint8_t*>(dmaRdMemPtrs[deviceIdx]);
    uint8_t* wrBufPtr = static_cast<uint8_t*>(dmaWrMemPtrs[deviceIdx]);

    for (int nodeIdx = 0; nodeIdx < kNodeCount; ++nodeIdx) {
      // NOTE: host_phys_addr should be handled in SysEmu, userspace should not fill this value
      wrList[nodeIdx] = {.src_host_virt_addr = templ::bit_cast<uint64_t>(wrBufPtr),
                         .src_host_phy_addr = templ::bit_cast<uint64_t>(wrBufPtr),
                         .dst_device_phy_addr = (nodeIdx == 0) ? 0 : getDmaWriteAddr(deviceIdx, kBufSize),
                         .size = kBufSize};
      wrBufPtr += kBufSize;

      rdList[nodeIdx] = {.dst_host_virt_addr = templ::bit_cast<uint64_t>(rdBufPtr),
                         .dst_host_phy_addr = templ::bit_cast<uint64_t>(rdBufPtr),
                         .src_device_phy_addr = (nodeIdx == 0) ? 0 : getDmaReadAddr(deviceIdx, kBufSize),
                         .size = kBufSize};
      rdBufPtr += kBufSize;
    }
    stream.push_back(
      IDevOpsApiCmd::createDmaWriteListCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, wrList, kNodeCount,
                                           device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_INVALID_ADDRESS));
    stream.push_back(
      IDevOpsApiCmd::createDmaReadListCmd(device_ops_api::CMD_FLAGS_BARRIER_ENABLE, rdList, kNodeCount,
                                          device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_INVALID_ADDRESS));
    // Move stream of commands to streams_[queueId]
    streams_.emplace(key(deviceIdx, queueIdx), std::move(stream));
  }
  close(randFd);

  executeAsync();
}

/**********************************************************
 *                                                         *
 *             DMA Stress Testing Functions                *
 *                                                         *
 **********************************************************/
void TestDevOpsApiDmaCmds::dataWRStressSize_2_1(uint8_t maxExp2) {
  std::vector<std::vector<uint8_t>> writtenBuffs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  int deviceCount = 1;
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    // This test is intended to execute different size of buffers on single queue.
    // This test exeutes commands and responses synchronously i.e. next command is sent after the response of previous
    // command is received.
    uint8_t queueCount = 1;
    std::vector<uint8_t> dmaRdBuf;
    std::vector<uint8_t> dmaWrBuf;

    for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
      for (uint8_t n = 2; n <= maxExp2; n++) {
        const uint64_t bufSize = 1ULL << n;
        dmaWrBuf.resize(bufSize, 0);
        for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
          dmaWrBuf[i] = i % 0x100;
        }
        auto devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
        auto hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
        auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
        stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                           hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                           device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
        writtenBuffs.push_back(std::move(dmaWrBuf));
      }

      for (uint8_t n = 2; n <= maxExp2; n++) {
        const uint64_t bufSize = 1ULL << n;
        dmaRdBuf.resize(bufSize, 0);
        auto devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
        auto hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
        auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
        stream.push_back(IDevOpsApiCmd::createDataReadCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                          hostVirtAddr, hostPhysAddr, dmaRdBuf.size(),
                                                          device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
        dmaRdBufs.push_back(std::move(dmaRdBuf));
      }

      // Move stream of commands to streams_
      streams_.emplace(key(deviceIdx, queueId), std::move(stream));
    }
  }
  executeSync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  for (std::size_t i = 0; i < writtenBuffs.size(); ++i) {
    EXPECT_EQ(writtenBuffs[i], dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataWRStressSpeed_2_2(uint8_t maxExp2) {
  std::vector<std::vector<uint8_t>> writtenBuffs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  int deviceCount = 1;
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    // This test is intended to execute different size of buffers on single queue.
    // This test exeutes commands and responses asynchronously i.e. commands and responses are processed using seprate
    // threads.
    uint8_t queueCount = 1;
    std::vector<uint8_t> dmaRdBuf;
    std::vector<uint8_t> dmaWrBuf;

    for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
      for (uint8_t n = 2; n <= maxExp2; n++) {
        const uint64_t bufSize = 1ULL << n;
        dmaWrBuf.resize(bufSize, 0);
        for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
          dmaWrBuf[i] = i % 0x100;
        }
        auto devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
        auto hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
        auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
        stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                           hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                           device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
        writtenBuffs.push_back(std::move(dmaWrBuf));
      }

      bool isFirstCommand = true; // Barrier for the first command
      for (uint8_t n = 2; n <= maxExp2; n++) {
        const uint64_t bufSize = 1ULL << n;
        dmaRdBuf.resize(bufSize, 0);
        auto devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
        auto hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
        auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
        stream.push_back(IDevOpsApiCmd::createDataReadCmd(isFirstCommand, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                          dmaRdBuf.size(),
                                                          device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
        dmaRdBufs.push_back(std::move(dmaRdBuf));
        isFirstCommand = false;
      }

      // Move stream of commands to streams_
      streams_.emplace(key(deviceIdx, queueId), std::move(stream));
    }
  }
  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  for (std::size_t i = 0; i < writtenBuffs.size(); ++i) {
    EXPECT_EQ(writtenBuffs[i], dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataWRStressChannelsSingleDeviceSingleQueue_2_3(uint32_t numOfLoopbackCmds) {
  const uint64_t bufSize = 4 * 1024; // 4K buffer size
  std::vector<uint8_t> dmaWrBuf(bufSize, 0);
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  int deviceCount = 1;
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    uint8_t queueCount = 1;

    for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = rand() % 0x100;
    }

    for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
      for (int i = 1; i <= (numOfLoopbackCmds / deviceCount) / queueCount; i++) {
        auto devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
        auto hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
        auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
        stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                           hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                           device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      }

      bool isFirstCommand = true; // Barrier for the first command
      for (int i = 1; i <= (numOfLoopbackCmds / deviceCount) / queueCount; i++) {
        std::vector<uint8_t> dmaRdBuf(bufSize, 0);
        auto devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
        auto hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
        auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
        stream.push_back(IDevOpsApiCmd::createDataReadCmd(isFirstCommand, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                          dmaRdBuf.size(),
                                                          device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
        dmaRdBufs.push_back(std::move(dmaRdBuf));
        isFirstCommand = false;
      }

      // Move stream of commands to streams_
      streams_.emplace(key(deviceIdx, queueId), std::move(stream));
    }
  }
  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  for (std::size_t i = 0; i < dmaRdBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBuf, dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataWRStressChannelsSingleDeviceMultiQueue_2_4(uint32_t numOfLoopbackCmds) {
  const uint64_t bufSize = 4 * 1024; // 4K buffer size
  std::vector<uint8_t> dmaWrBuf(bufSize, 0);
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  int deviceCount = 1;
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    uint8_t queueCount = getSqCount(deviceIdx);

    for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = rand() % 0x100;
    }

    for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
      for (int i = 1; i <= (numOfLoopbackCmds / deviceCount) / queueCount; i++) {
        auto devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
        auto hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
        auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
        stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                           hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                           device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      }

      bool isFirstCommand = true; // Barrier for the first command
      for (int i = 1; i <= (numOfLoopbackCmds / deviceCount) / queueCount; i++) {
        std::vector<uint8_t> dmaRdBuf(bufSize, 0);
        auto devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
        auto hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
        auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
        stream.push_back(IDevOpsApiCmd::createDataReadCmd(isFirstCommand, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                          dmaRdBuf.size(),
                                                          device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
        dmaRdBufs.push_back(std::move(dmaRdBuf));
        isFirstCommand = false;
      }

      // Move stream of commands to streams_[queueId]
      streams_.emplace(key(deviceIdx, queueId), std::move(stream));
    }
  }
  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  for (std::size_t i = 0; i < dmaRdBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBuf, dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataWRStressChannelsMultiDeviceMultiQueue_2_5(uint32_t numOfLoopbackCmds) {
  const uint64_t bufSize = 4 * 1024; // 4K buffer size
  std::vector<uint8_t> dmaWrBuf(bufSize, 0);
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    uint8_t queueCount = getSqCount(deviceIdx);

    for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = rand() % 0x100;
    }

    for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
      for (int i = 1; i <= (numOfLoopbackCmds / deviceCount) / queueCount; i++) {
        auto devPhysAddr = getDmaWriteAddr(deviceIdx, dmaWrBuf.size());
        auto hostVirtAddr = templ::bit_cast<uint64_t>(dmaWrBuf.data());
        auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
        stream.push_back(IDevOpsApiCmd::createDataWriteCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devPhysAddr,
                                                           hostVirtAddr, hostPhysAddr, dmaWrBuf.size(),
                                                           device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      }

      bool isFirstCommand = true; // Barrier for the first command
      for (int i = 1; i <= (numOfLoopbackCmds / deviceCount) / queueCount; i++) {
        std::vector<uint8_t> dmaRdBuf(bufSize, 0);
        auto devPhysAddr = getDmaReadAddr(deviceIdx, dmaRdBuf.size());
        auto hostVirtAddr = templ::bit_cast<uint64_t>(dmaRdBuf.data());
        auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
        stream.push_back(IDevOpsApiCmd::createDataReadCmd(isFirstCommand, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                          dmaRdBuf.size(),
                                                          device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
        dmaRdBufs.push_back(std::move(dmaRdBuf));
        isFirstCommand = false;
      }

      // Move stream of commands to streams_[queueId]
      streams_.emplace(key(deviceIdx, queueId), std::move(stream));
    }
  }
  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  for (std::size_t i = 0; i < dmaRdBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBuf, dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dmaListWrAndRd(bool singleDevice, bool singleQueue, uint32_t numOfDmaEntries) {
  std::vector<void*> dmaWrMemPtrs;
  std::vector<void*> dmaRdMemPtrs;
  const uint8_t kMaxNodeCountPerCmd = 4;
  const size_t kBufSize = 4 * 1024; // 4K
  const size_t kMaxAllocSize =
    0x1 << 31; // 2GB, TODO: Get from device layer when support becomes available in device layer

  ASSERT_LE(numOfDmaEntries * kBufSize, kMaxAllocSize)
    << "Number of DMA entries are too large (max allowed: " << kMaxAllocSize / (numOfDmaEntries * kBufSize) << ")"
    << std::endl;

  auto deviceCount = (singleDevice) ? 1 : getDevicesCount();
  auto numOfDmaEntriesPerDev = numOfDmaEntries / deviceCount;

  device_ops_api::dma_write_node wrList[kMaxNodeCountPerCmd];
  device_ops_api::dma_read_node rdList[kMaxNodeCountPerCmd];

  auto randFd = open("/dev/urandom", O_RDONLY);
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;

    dmaWrMemPtrs.push_back(allocDmaBuffer(deviceIdx, numOfDmaEntriesPerDev * kBufSize, true));
    dmaRdMemPtrs.push_back(allocDmaBuffer(deviceIdx, numOfDmaEntriesPerDev * kBufSize, false));

    // Randomize write buffer
    read(randFd, dmaWrMemPtrs[deviceIdx], numOfDmaEntriesPerDev * kBufSize);

    uint8_t* rdBufPtr = static_cast<uint8_t*>(dmaRdMemPtrs[deviceIdx]);
    uint8_t* wrBufPtr = static_cast<uint8_t*>(dmaWrMemPtrs[deviceIdx]);

    auto queueCount = (singleQueue) ? 1 : getSqCount(deviceIdx);
    auto numOfDmaEntriesPerQueue = numOfDmaEntriesPerDev / queueCount;
    for (int queueIdx = 0; queueIdx < queueCount; ++queueIdx) {
      for (int i = 0; i < numOfDmaEntriesPerQueue;) {
        // NOTE: host_phys_addr should be handled in SysEmu, userspace should not fill this value
        wrList[i % kMaxNodeCountPerCmd] = {.src_host_virt_addr = templ::bit_cast<uint64_t>(wrBufPtr),
                                           .src_host_phy_addr = templ::bit_cast<uint64_t>(wrBufPtr),
                                           .dst_device_phy_addr = getDmaWriteAddr(deviceIdx, kBufSize),
                                           .size = kBufSize};
        wrBufPtr += kBufSize;
        ++i;
        if (i % kMaxNodeCountPerCmd == 0 || i == numOfDmaEntriesPerQueue) {
          stream.push_back(IDevOpsApiCmd::createDmaWriteListCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, wrList,
                                                                (i - 1) % kMaxNodeCountPerCmd + 1,
                                                                device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
        }
      }

      for (int i = 0; i < numOfDmaEntriesPerQueue;) {
        // NOTE: host_phys_addr should be handled in SysEmu, userspace should not fill this value
        rdList[i % kMaxNodeCountPerCmd] = {.dst_host_virt_addr = templ::bit_cast<uint64_t>(rdBufPtr),
                                           .dst_host_phy_addr = templ::bit_cast<uint64_t>(rdBufPtr),
                                           .src_device_phy_addr = getDmaReadAddr(deviceIdx, kBufSize),
                                           .size = kBufSize};
        rdBufPtr += kBufSize;
        ++i;
        if (i % kMaxNodeCountPerCmd == 0 || i == numOfDmaEntriesPerQueue) {
          stream.push_back(IDevOpsApiCmd::createDmaReadListCmd(
            (i == 1) ? device_ops_api::CMD_FLAGS_BARRIER_ENABLE : device_ops_api::CMD_FLAGS_BARRIER_DISABLE, rdList,
            (i - 1) % kMaxNodeCountPerCmd + 1, device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
        }
      }
      // Move stream of commands to streams_[queueId]
      streams_.emplace(key(deviceIdx, queueIdx), std::move(stream));
    }
  }
  close(randFd);

  executeAsync();

  // Validate data received from DMA
  for (int deviceIdx = 0; deviceIdx < deviceCount; ++deviceIdx) {
    // Skip data validation in case of loopback driver
    if (!FLAGS_loopback_driver) {
      auto queueCount = (singleQueue) ? 1 : getSqCount(deviceIdx);
      auto numOfDmaEntriesPerQueue = numOfDmaEntriesPerDev / queueCount;
      auto bytesWritten = numOfDmaEntriesPerQueue * queueCount * kBufSize;
      EXPECT_EQ(memcmp(dmaWrMemPtrs[deviceIdx], dmaRdMemPtrs[deviceIdx], bytesWritten), 0)
        << "DMA W/R data mismatched for deviceIdx: " << (int)deviceIdx << std::endl;
    }
    freeDmaBuffer(dmaWrMemPtrs[deviceIdx]);
    freeDmaBuffer(dmaRdMemPtrs[deviceIdx]);
  }
}
