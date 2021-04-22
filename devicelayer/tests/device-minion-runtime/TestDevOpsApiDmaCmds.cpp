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

/**********************************************************
 *                                                         *
 *              DMA Basic Testing Functions                *
 *                                                         *
 **********************************************************/
void TestDevOpsApiDmaCmds::dataRWCmdWithBasicCmds_3_4() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  uint8_t queueCount = devLayer_->getSubmissionQueuesCount(kIDevice);
  initTagId(0x51);
  const uint64_t bufSize = 64;
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  std::vector<uint8_t> dmaWrBuf;
  std::vector<uint8_t> dmaRdBuf;

  for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
    dmaWrBuf.resize(bufSize, 0);
    for (int i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = rand() % 0x100;
    }
    stream.push_back(IDevOpsApiCmd::createEchoCmd(getNextTagId(), false, kEchoPayload));
    stream.push_back(
      IDevOpsApiCmd::createApiCompatibilityCmd(getNextTagId(), false, kDevFWMajor, kDevFWMinor, kDevFWPatch));
    stream.push_back(IDevOpsApiCmd::createFwVersionCmd(getNextTagId(), false, 1));

    auto devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
    auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
    auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                       dmaWrBuf.size(),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaWrBufs.push_back(std::move(dmaWrBuf));
    dmaRdBuf.resize(bufSize, 0);
    devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                      dmaRdBuf.size(),
                                                      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

    dmaRdBufs.push_back(std::move(dmaRdBuf));
    // Move stream of commands to streams_[queueId]
    streams_.emplace(queueId, std::move(stream));
  }

  executeAsync();
  // For DMA received Data Validation
  for (size_t i = 0; i < dmaWrBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
  }
}

// Read/Write access mixed b2b .. i.e. DMA WRITE, DMA READ,
// DMA READ, DMA WRITE. The address would be orthogonal to each other.
void TestDevOpsApiDmaCmds::dataRWCmdMixed_3_5() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  uint8_t queueCount = devLayer_->getSubmissionQueuesCount(kIDevice);
  initTagId(0x61);
  const int buffSizeA = 512;
  const int buffSizeB = 256;
  // Create some buffers for send and receiving data
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  std::vector<uint8_t> dmaWrBuf;
  std::vector<uint8_t> dmaRdBuf;

  for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
    dmaWrBuf.resize(buffSizeA, 0);
    for (int i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = rand() % 0x100;
    }
    auto devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
    auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
    auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                       dmaWrBuf.size(),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaWrBufs.push_back(std::move(dmaWrBuf));

    dmaWrBuf.resize(buffSizeA);
    for (int i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = rand() % 0x100;
    }
    devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                       dmaWrBuf.size(),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaWrBufs.push_back(std::move(dmaWrBuf));

    dmaRdBuf.resize(buffSizeA, 0);
    devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                      dmaRdBuf.size(),
                                                      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaRdBufs.push_back(std::move(dmaRdBuf));

    dmaRdBuf.resize(buffSizeA, 0);
    devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                      dmaRdBuf.size(),
                                                      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaRdBufs.push_back(std::move(dmaRdBuf));

    dmaWrBuf.resize(buffSizeB, 0);
    for (int i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = rand() % 0x100;
    }
    devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                       dmaWrBuf.size(),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaWrBufs.push_back(std::move(dmaWrBuf));

    dmaRdBuf.resize(buffSizeB, 0);
    devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                      dmaRdBuf.size(),
                                                      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaRdBufs.push_back(std::move(dmaRdBuf));

    // Move stream of commands to streams_[queueId]
    streams_.emplace(queueId, std::move(stream));
  }

  executeAsync();

  // For DMA received Data Validation
  for (std::size_t i = 0; i < dmaWrBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
  }
}

// Add variable sized to each transaction, you can pick from
// an array of 5-10 different sizes randomly, and assign to
// the different channels different sizes
void TestDevOpsApiDmaCmds::dataRWCmdMixedWithVarSize_3_6() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  uint8_t queueCount = devLayer_->getSubmissionQueuesCount(kIDevice);
  initTagId(0x71);

  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
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
    auto devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
    auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
    auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                       dmaWrBuf.size(),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaWrBufs.push_back(std::move(dmaWrBuf));

    // Data Write Cmd2 with bufSize[1]
    dmaWrBuf.resize(bufSize[1], 0);
    for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = rand() % 0x100;
    }
    devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                       dmaWrBuf.size(),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaWrBufs.push_back(std::move(dmaWrBuf));

    // Data Read Cmd1 with bufSize[0]
    devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                      dmaRdBuf.size(),
                                                      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaRdBufs.push_back(std::move(dmaRdBuf));

    // Data Read Cmd2 with bufSize[1]
    dmaRdBuf.resize(bufSize[1], 0);
    devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                      dmaRdBuf.size(),
                                                      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaRdBufs.push_back(std::move(dmaRdBuf));

    // Data Write Cmd3 with bufSize[2]
    dmaWrBuf.resize(bufSize[2], 0);
    for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = rand() % 0x100;
    }
    devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                       dmaWrBuf.size(),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaWrBufs.push_back(std::move(dmaWrBuf));

    // Data Read Cmd3 with bufSize[2]
    dmaRdBuf.resize(bufSize[2], 0);
    devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                      dmaRdBuf.size(),
                                                      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaRdBufs.push_back(std::move(dmaRdBuf));

    // Move stream of commands to streams_[queueId]
    streams_.emplace(queueId, std::move(stream));
  }

  executeAsync();

  // For DMA received Data Validation
  for (std::size_t i = 0; i < dmaWrBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
  }
}

// Add minimal 8 DMA transactions with each channel
// having different transfer size, with at least
// 1 CMD in the middle having a Barrier bit set
void TestDevOpsApiDmaCmds::dataRWCmdAllChannels_3_7() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  uint8_t queueCount = devLayer_->getSubmissionQueuesCount(kIDevice);
  initTagId(0x81);

  const uint64_t bufSize[4] = {256, 512, 1024, 2048};
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;

  for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
    std::vector<uint8_t> dmaWrBuf(bufSize[0], 0);
    std::vector<uint8_t> dmaRdBuf(bufSize[0], 0);
    // Data Write Cmd1 with bufSize[0]
    for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = rand() % 0x100;
    }
    auto devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
    auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
    auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                       dmaWrBuf.size(),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaWrBufs.push_back(std::move(dmaWrBuf));

    // Data Write Cmd2 with bufSize[1]
    dmaWrBuf.resize(bufSize[1], 0);
    for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = rand() % 0x100;
    }
    devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                       dmaWrBuf.size(),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaWrBufs.push_back(std::move(dmaWrBuf));

    // Data Write Cmd3 with bufSize[2]
    dmaWrBuf.resize(bufSize[2], 0);
    for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = rand() % 0x100;
    }
    devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                       dmaWrBuf.size(),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaWrBufs.push_back(std::move(dmaWrBuf));

    // Data Write Cmd4 with bufSize[3]
    dmaWrBuf.resize(bufSize[3], 0);
    for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = rand() % 0x100;
    }
    devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                       dmaWrBuf.size(),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaWrBufs.push_back(std::move(dmaWrBuf));

    // Data Read Cmd1 with bufSize[0] with barrier
    devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                      dmaRdBuf.size(),
                                                      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaRdBufs.push_back(std::move(dmaRdBuf));

    // Data Read Cmd2 with bufSize[1]
    dmaRdBuf.resize(bufSize[1], 0);
    devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                      dmaRdBuf.size(),
                                                      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaRdBufs.push_back(std::move(dmaRdBuf));

    // Data Read Cmd3 with bufSize[2]
    dmaRdBuf.resize(bufSize[2], 0);
    devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                      dmaRdBuf.size(),
                                                      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaRdBufs.push_back(std::move(dmaRdBuf));

    // Data Read Cmd4 with bufSize[3]
    dmaRdBuf.resize(bufSize[3], 0);
    devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                      dmaRdBuf.size(),
                                                      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaRdBufs.push_back(std::move(dmaRdBuf));

    // Move stream of commands to streams_[queueId]
    streams_.emplace(queueId, std::move(stream));
  }

  executeAsync();

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
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  uint8_t queueCount = devLayer_->getSubmissionQueuesCount(kIDevice);
  initTagId(0x31);

  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  std::vector<uint8_t> dmaRdBuf;
  std::vector<uint8_t> dmaWrBuf;
  const uint64_t bufSize = 64;

  for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
    dmaWrBuf.resize(bufSize, 0);
    for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = static_cast<uint8_t>(rand() % 0x100);
    }
    auto devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
    auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
    auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                       dmaWrBuf.size(),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaWrBufs.push_back(std::move(dmaWrBuf));

    // Read data
    dmaRdBuf.resize(bufSize, 0);
    devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                      dmaRdBuf.size(),
                                                      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaRdBufs.push_back(std::move(dmaRdBuf));

    // Move stream of commands to streams_[queueId]
    streams_.emplace(queueId, std::move(stream));
  }

  executeAsync();

  // Validate data received from DMA
  for (std::size_t i = 0; i < dmaWrBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataRWCmdWithBarrier_PositiveTesting_3_10() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0x41);
  uint8_t queueCount = devLayer_->getSubmissionQueuesCount(kIDevice);
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;

  const uint64_t dmaWrBufsize = 512;
  const uint64_t dmaRdBufsize = 4 * dmaWrBufsize;
  std::vector<uint8_t> dmaWrBuf;
  std::vector<uint8_t> dmaRdBuf;

  for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {

    dmaWrBuf.resize(dmaWrBufsize, 0);
    for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = rand() % 0x100;
    }
    auto devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
    auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
    auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                       dmaWrBuf.size(),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaWrBufs.push_back(std::move(dmaWrBuf));

    dmaWrBuf.resize(dmaWrBufsize, 0);
    for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = rand() % 0x100;
    }
    devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                       dmaWrBuf.size(),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaWrBufs.push_back(std::move(dmaWrBuf));

    dmaWrBuf.resize(dmaWrBufsize, 0);
    for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = rand() % 0x100;
    }
    devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                       dmaWrBuf.size(),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaWrBufs.push_back(std::move(dmaWrBuf));

    dmaWrBuf.resize(dmaWrBufsize, 0);
    for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
      dmaWrBuf[i] = rand() % 0x100;
    }
    devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                       dmaWrBuf.size(),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaWrBufs.push_back(std::move(dmaWrBuf));

    // since we sent 4 DMA write cmd, we need to read back all the data in a single DMA read cmd
    dmaRdBuf.resize(dmaRdBufsize, 0);
    devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
    hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                      dmaRdBuf.size(),
                                                      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    dmaRdBufs.push_back(std::move(dmaRdBuf));

    // Move stream of commands to streams_[queueId]
    streams_.emplace(queueId, std::move(stream));
  }

  executeAsync();

  // For DMA received Data Validation
  for (uint8_t queueId = 0; queueId < queueCount; queueId++) {
    uint8_t writeIdxOffset = queueId * 4;
    EXPECT_EQ(
      dmaWrBufs[writeIdxOffset],
      std::vector<uint8_t>(dmaRdBufs[queueId].begin(), dmaRdBufs[queueId].begin() + dmaWrBufs[writeIdxOffset].size()));
    EXPECT_EQ(dmaWrBufs[writeIdxOffset + 1],
              std::vector<uint8_t>(dmaRdBufs[queueId].begin() + dmaWrBufs[writeIdxOffset].size(),
                                   dmaRdBufs[queueId].begin() + dmaWrBufs[writeIdxOffset].size() +
                                     dmaWrBufs[writeIdxOffset + 1].size()));
    EXPECT_EQ(dmaWrBufs[writeIdxOffset + 2],
              std::vector<uint8_t>(dmaRdBufs[queueId].begin() + dmaWrBufs[writeIdxOffset].size() +
                                     dmaWrBufs[writeIdxOffset + 1].size(),
                                   dmaRdBufs[queueId].begin() + dmaWrBufs[writeIdxOffset].size() +
                                     dmaWrBufs[writeIdxOffset + 1].size() + dmaWrBufs[writeIdxOffset + 2].size()));
    EXPECT_EQ(dmaWrBufs[writeIdxOffset + 3],
              std::vector<uint8_t>(dmaRdBufs[queueId].begin() + dmaWrBufs[writeIdxOffset].size() +
                                     dmaWrBufs[writeIdxOffset + 1].size() + dmaWrBufs[writeIdxOffset + 2].size(),
                                   dmaRdBufs[queueId].end()));
  }
}

/**********************************************************
 *                                                         *
 *             DMA Stress Testing Functions                *
 *                                                         *
 **********************************************************/
void TestDevOpsApiDmaCmds::dataWRStressSize_2_1(uint8_t maxExp2) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  // This test is intended to execute different size of buffers on single queue.
  // This test exeutes commands and responses synchronously i.e. next command is sent after the response of previous
  // command is received.
  uint8_t queueCount = 1;
  initTagId(0x91);

  std::vector<std::vector<uint8_t>> writtenBuffs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  std::vector<uint8_t> dmaRdBuf;
  std::vector<uint8_t> dmaWrBuf;

  for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
    for (uint8_t n = 2; n <= maxExp2; n++) {
      const uint64_t bufSize = 1ULL << n;
      dmaWrBuf.resize(bufSize, 0);
      for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = i % 0x100;
      }
      auto devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
      auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                         dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      writtenBuffs.push_back(std::move(dmaWrBuf));
    }

    for (uint8_t n = 2; n <= maxExp2; n++) {
      const uint64_t bufSize = 1ULL << n;
      dmaRdBuf.resize(bufSize, 0);
      auto devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
      auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                        dmaRdBuf.size(),
                                                        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaRdBufs.push_back(std::move(dmaRdBuf));
    }

    // Move stream of commands to streams_[queueId]
    streams_.emplace(queueId, std::move(stream));
  }

  executeSync();

  for (std::size_t i = 0; i < writtenBuffs.size(); ++i) {
    EXPECT_EQ(writtenBuffs[i], dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataWRStressSpeed_2_2(uint8_t maxExp2) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  // This test is intended to execute different size of buffers on single queue.
  // This test exeutes commands and responses asynchronously i.e. commands and responses are processed using seprate
  // threads.
  uint8_t queueCount = 1;
  initTagId(0x101);

  std::vector<std::vector<uint8_t>> writtenBuffs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  std::vector<uint8_t> dmaRdBuf;
  std::vector<uint8_t> dmaWrBuf;

  for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
    for (uint8_t n = 2; n <= maxExp2; n++) {
      const uint64_t bufSize = 1ULL << n;
      dmaWrBuf.resize(bufSize, 0);
      for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
        dmaWrBuf[i] = i % 0x100;
      }
      auto devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
      auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                         dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      writtenBuffs.push_back(std::move(dmaWrBuf));
    }

    bool isFirstCommand = true; // Barrier for the first command
    for (uint8_t n = 2; n <= maxExp2; n++) {
      const uint64_t bufSize = 1ULL << n;
      dmaRdBuf.resize(bufSize, 0);
      auto devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
      auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), isFirstCommand, devPhysAddr, hostVirtAddr,
                                                        hostPhysAddr, dmaRdBuf.size(),
                                                        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaRdBufs.push_back(std::move(dmaRdBuf));
      isFirstCommand = false;
    }

    // Move stream of commands to streams_[queueId]
    streams_.emplace(queueId, std::move(stream));
  }

  executeAsync();

  for (std::size_t i = 0; i < writtenBuffs.size(); ++i) {
    EXPECT_EQ(writtenBuffs[i], dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataWRStressChannelsSingleQueue_2_3(uint32_t numOfLoopbackCmds) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  uint8_t queueCount = 1;
  initTagId(0x111);

  const uint64_t bufSize = 4 * 1024; // 4K buffer size
  std::vector<uint8_t> dmaWrBuf(bufSize, 0);
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
    dmaWrBuf[i] = rand() % 0x100;
  }

  for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
    for (int i = 1; i <= numOfLoopbackCmds / queueCount; i++) {
      auto devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
      auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                         dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    }

    bool isFirstCommand = true; // Barrier for the first command
    for (int i = 1; i <= numOfLoopbackCmds / queueCount; i++) {
      std::vector<uint8_t> dmaRdBuf(bufSize, 0);
      auto devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
      auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), isFirstCommand, devPhysAddr, hostVirtAddr,
                                                        hostPhysAddr, dmaRdBuf.size(),
                                                        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaRdBufs.push_back(std::move(dmaRdBuf));
      isFirstCommand = false;
    }

    // Move stream of commands to streams_[queueId]
    streams_.emplace(queueId, std::move(stream));
  }

  executeAsync();

  for (std::size_t i = 0; i < dmaRdBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBuf, dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataWRStressChannelsMultiQueue_2_4(uint32_t numOfLoopbackCmds) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  uint8_t queueCount = devLayer_->getSubmissionQueuesCount(kIDevice);
  initTagId(0x111);

  const uint64_t bufSize = 4 * 1024; // 4K buffer size
  std::vector<uint8_t> dmaWrBuf(bufSize, 0);
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
    dmaWrBuf[i] = rand() % 0x100;
  }

  for (uint8_t queueId = 0; queueId < queueCount; ++queueId) {
    for (int i = 1; i <= numOfLoopbackCmds / queueCount; i++) {
      auto devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
      auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                         dmaWrBuf.size(),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    }

    bool isFirstCommand = true; // Barrier for the first command
    for (int i = 1; i <= numOfLoopbackCmds / queueCount; i++) {
      std::vector<uint8_t> dmaRdBuf(bufSize, 0);
      auto devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
      auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), isFirstCommand, devPhysAddr, hostVirtAddr,
                                                        hostPhysAddr, dmaRdBuf.size(),
                                                        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      dmaRdBufs.push_back(std::move(dmaRdBuf));
      isFirstCommand = false;
    }

    // Move stream of commands to streams_[queueId]
    streams_.emplace(queueId, std::move(stream));
  }

  executeAsync();

  for (std::size_t i = 0; i < dmaRdBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBuf, dmaRdBufs[i]);
  }
}
