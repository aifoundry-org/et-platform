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
  initTagId(0x51);

  stream.push_back(IDevOpsApiCmd::createEchoCmd(getNextTagId(), false, kEchoPayload));
  stream.push_back(
    IDevOpsApiCmd::createApiCompatibilityCmd(getNextTagId(), false, kDevFWMajor, kDevFWMinor, kDevFWPatch));
  stream.push_back(IDevOpsApiCmd::createFwVersionCmd(getNextTagId(), false, 1));

  const uint64_t bufSize = 64;
  std::vector<uint8_t> dmaBuf1(bufSize, 0);
  for (std::size_t i = 0; i < dmaBuf1.size(); ++i) {
    dmaBuf1[i] = i % 0x100;
  }
  auto devPhysAddr = getDmaWriteAddr(dmaBuf1.size());
  auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf1.data());
  auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                     dmaBuf1.size(),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  std::vector<uint8_t> dmaBuf2(bufSize, 0);
  devPhysAddr = getDmaReadAddr(dmaBuf2.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf2.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                    dmaBuf2.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  // For DMA received Data Validation
  EXPECT_EQ(dmaBuf1, dmaBuf2);
}

// Read/Write access mixed b2b .. i.e. DMA WRITE, DMA READ,
// DMA READ, DMA WRITE. The address would be orthogonal to each other.
void TestDevOpsApiDmaCmds::dataRWCmdMixed_3_5() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0x61);

  const uint64_t bufSizeA = 512;
  const uint64_t bufSizeB = 256;

  // Data Write Cmd1 with bufSizeA
  std::vector<uint8_t> dmaBuf1(bufSizeA, 0);
  for (std::size_t i = 0; i < dmaBuf1.size(); ++i) {
    dmaBuf1[i] = i % 0x100;
  }
  auto devPhysAddr = getDmaWriteAddr(dmaBuf1.size());
  auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf1.data());
  auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                     dmaBuf1.size(),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Data Write Cmd2 with bufSizeA
  std::vector<uint8_t> dmaBuf2(bufSizeA, 0);
  for (std::size_t i = 0; i < dmaBuf2.size(); ++i) {
    dmaBuf2[i] = i % 0x100;
  }
  devPhysAddr = getDmaWriteAddr(dmaBuf2.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf2.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                     dmaBuf2.size(),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Data Read Cmd1 with bufSizeA
  std::vector<uint8_t> dmaBuf3(bufSizeA, 0);
  devPhysAddr = getDmaReadAddr(dmaBuf3.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf3.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                    dmaBuf3.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Data Read Cmd2 with bufSizeA
  std::vector<uint8_t> dmaBuf4(bufSizeA, 0);
  devPhysAddr = getDmaReadAddr(dmaBuf4.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf4.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                    dmaBuf4.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Data Write Cmd3 with bufSizeB
  std::vector<uint8_t> dmaBuf5(bufSizeB, 0);
  for (std::size_t i = 0; i < dmaBuf5.size(); ++i) {
    dmaBuf5[i] = i % 0x100;
  }
  devPhysAddr = getDmaWriteAddr(dmaBuf5.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf5.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                     dmaBuf5.size(),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Data Read Cmd3 with bufSizeB
  std::vector<uint8_t> dmaBuf6(bufSizeB, 0);
  devPhysAddr = getDmaReadAddr(dmaBuf6.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf6.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                    dmaBuf6.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  // For DMA received Data Validation
  EXPECT_EQ(dmaBuf1, dmaBuf3);
  EXPECT_EQ(dmaBuf2, dmaBuf4);
  EXPECT_EQ(dmaBuf5, dmaBuf6);
}

// Add variable sized to each transaction, you can pick from
// an array of 5-10 different sizes randomly, and assign to
// the different channels different sizes
void TestDevOpsApiDmaCmds::dataRWCmdMixedWithVarSize_3_6() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0x71);

  static const std::array<uint64_t, 10> bufSizeArray = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
  srand(time(0));
  uint64_t bufSize[3];
  bufSize[0] = bufSizeArray[rand() % bufSizeArray.size()];
  bufSize[1] = bufSizeArray[rand() % bufSizeArray.size()];
  bufSize[2] = bufSizeArray[rand() % bufSizeArray.size()];
  TEST_VLOG(1) << "rwBuffer Values are " << bufSize[0] << ", " << bufSize[1] << ", " << bufSize[2] << std::endl;

  // Data Write Cmd1 with bufSize[0]
  std::vector<uint8_t> dmaBuf1(bufSize[0], 0);
  for (std::size_t i = 0; i < dmaBuf1.size(); ++i) {
    dmaBuf1[i] = i % 0x100;
  }
  auto devPhysAddr = getDmaWriteAddr(dmaBuf1.size());
  auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf1.data());
  auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                     dmaBuf1.size(),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Data Write Cmd2 with bufSize[1]
  std::vector<uint8_t> dmaBuf2(bufSize[1], 0);
  for (std::size_t i = 0; i < dmaBuf2.size(); ++i) {
    dmaBuf2[i] = i % 0x100;
  }
  devPhysAddr = getDmaWriteAddr(dmaBuf2.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf2.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                     dmaBuf2.size(),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Data Read Cmd1 with bufSize[0]
  std::vector<uint8_t> dmaBuf3(bufSize[0], 0);
  devPhysAddr = getDmaReadAddr(dmaBuf3.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf3.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                    dmaBuf3.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Data Read Cmd2 with bufSize[1]
  std::vector<uint8_t> dmaBuf4(bufSize[1], 0);
  devPhysAddr = getDmaReadAddr(dmaBuf4.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf4.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                    dmaBuf4.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Data Write Cmd3 with bufSize[2]
  std::vector<uint8_t> dmaBuf5(bufSize[2], 0);
  for (std::size_t i = 0; i < dmaBuf5.size(); ++i) {
    dmaBuf5[i] = i % 0x100;
  }
  devPhysAddr = getDmaWriteAddr(dmaBuf5.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf5.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                     dmaBuf5.size(),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Data Read Cmd3 with bufSize[2]
  std::vector<uint8_t> dmaBuf6(bufSize[2], 0);
  devPhysAddr = getDmaReadAddr(dmaBuf6.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf6.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                    dmaBuf6.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  // For DMA received Data Validation
  EXPECT_EQ(dmaBuf1, dmaBuf3);
  EXPECT_EQ(dmaBuf2, dmaBuf4);
  EXPECT_EQ(dmaBuf5, dmaBuf6);
}

// Add minimal 8 DMA transactions with each channel
// having different transfer size, with at least
// 1 CMD in the middle having a Barrier bit set
void TestDevOpsApiDmaCmds::dataRWCmdAllChannels_3_7() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0x81);

  const uint64_t bufSize[4] = {256, 512, 1024, 2048};

  // Data Write Cmd1 with bufSize[0]
  std::vector<uint8_t> dmaBuf1(bufSize[0], 0);
  for (std::size_t i = 0; i < dmaBuf1.size(); ++i) {
    dmaBuf1[i] = i % 0x100;
  }
  auto devPhysAddr = getDmaWriteAddr(dmaBuf1.size());
  auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf1.data());
  auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                     dmaBuf1.size(),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Data Write Cmd2 with bufSize[1]
  std::vector<uint8_t> dmaBuf2(bufSize[1], 0);
  for (std::size_t i = 0; i < dmaBuf2.size(); ++i) {
    dmaBuf2[i] = i % 0x100;
  }
  devPhysAddr = getDmaWriteAddr(dmaBuf2.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf2.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                     dmaBuf2.size(),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Data Write Cmd3 with bufSize[2]
  std::vector<uint8_t> dmaBuf3(bufSize[2], 0);
  for (std::size_t i = 0; i < dmaBuf3.size(); ++i) {
    dmaBuf3[i] = i % 0x100;
  }
  devPhysAddr = getDmaWriteAddr(dmaBuf3.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf3.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                     dmaBuf3.size(),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Data Write Cmd4 with bufSize[3]
  std::vector<uint8_t> dmaBuf4(bufSize[3], 0);
  for (std::size_t i = 0; i < dmaBuf4.size(); ++i) {
    dmaBuf4[i] = i % 0x100;
  }
  devPhysAddr = getDmaWriteAddr(dmaBuf4.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf4.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                     dmaBuf4.size(),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Data Read Cmd1 with bufSize[0] with barrier
  std::vector<uint8_t> dmaBuf5(bufSize[0], 0);
  devPhysAddr = getDmaReadAddr(dmaBuf5.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf5.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                    dmaBuf5.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Data Read Cmd2 with bufSize[1]
  std::vector<uint8_t> dmaBuf6(bufSize[1], 0);
  devPhysAddr = getDmaReadAddr(dmaBuf6.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf6.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                    dmaBuf6.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Data Read Cmd3 with bufSize[2]
  std::vector<uint8_t> dmaBuf7(bufSize[2], 0);
  devPhysAddr = getDmaReadAddr(dmaBuf7.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf7.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                    dmaBuf7.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Data Read Cmd4 with bufSize[3]
  std::vector<uint8_t> dmaBuf8(bufSize[3], 0);
  devPhysAddr = getDmaReadAddr(dmaBuf8.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf8.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                    dmaBuf8.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  // For DMA received Data Validation
  EXPECT_EQ(dmaBuf1, dmaBuf5);
  EXPECT_EQ(dmaBuf2, dmaBuf6);
  EXPECT_EQ(dmaBuf3, dmaBuf7);
  EXPECT_EQ(dmaBuf4, dmaBuf8);
}

/**********************************************************
 *                                                         *
 *             DMA Positive Testing Functions              *
 *                                                         *
 **********************************************************/
void TestDevOpsApiDmaCmds::dataRWCmd_PositiveTesting_3_1() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0x31);

  const uint64_t bufSize = 64;
  std::vector<uint8_t> dmaBuf1(bufSize, 0);
  for (std::size_t i = 0; i < dmaBuf1.size(); ++i) {
    dmaBuf1[i] = i % 0x100;
  }
  auto devPhysAddr = getDmaWriteAddr(dmaBuf1.size());
  auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf1.data());
  auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                     dmaBuf1.size(),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  std::vector<uint8_t> dmaBuf2(bufSize, 0);
  devPhysAddr = getDmaReadAddr(dmaBuf2.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf2.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                    dmaBuf2.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  EXPECT_EQ(dmaBuf1, dmaBuf2);
}

void TestDevOpsApiDmaCmds::dataRWCmdWithBarrier_PositiveTesting_3_10() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0x41);

  const uint64_t writeBufSize = 512;
  const uint64_t readBufSize = 4 * writeBufSize;
  std::vector<uint8_t> dmaBuf1(writeBufSize, 0);
  for (std::size_t i = 0; i < dmaBuf1.size(); ++i) {
    dmaBuf1[i] = i % 0x100;
  }
  auto devPhysAddr = getDmaWriteAddr(dmaBuf1.size());
  auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf1.data());
  auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                     dmaBuf1.size(),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  std::vector<uint8_t> dmaBuf2(writeBufSize, 0);
  for (std::size_t i = 0; i < dmaBuf2.size(); ++i) {
    dmaBuf2[i] = i % 0x100;
  }
  devPhysAddr = getDmaWriteAddr(dmaBuf2.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf2.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                     dmaBuf2.size(),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  std::vector<uint8_t> dmaBuf3(writeBufSize, 0);
  for (std::size_t i = 0; i < dmaBuf3.size(); ++i) {
    dmaBuf3[i] = i % 0x100;
  }
  devPhysAddr = getDmaWriteAddr(dmaBuf3.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf3.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                     dmaBuf3.size(),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  std::vector<uint8_t> dmaBuf4(writeBufSize, 0);
  for (std::size_t i = 0; i < dmaBuf4.size(); ++i) {
    dmaBuf4[i] = i % 0x100;
  }
  devPhysAddr = getDmaWriteAddr(dmaBuf4.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf4.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                     dmaBuf4.size(),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // since we sent 4 DMA write cmd, we need to read back all the data in a single DMA read cmd
  std::vector<uint8_t> dmaBuf5(readBufSize, 0);
  devPhysAddr = getDmaReadAddr(dmaBuf5.size());
  hostVirtAddr = reinterpret_cast<uint64_t>(dmaBuf5.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                    dmaBuf5.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  // For DMA received Data Validation
  EXPECT_EQ(dmaBuf1, std::vector<uint8_t>(dmaBuf5.begin(), dmaBuf5.begin() + dmaBuf1.size()));
  EXPECT_EQ(dmaBuf2,
            std::vector<uint8_t>(dmaBuf5.begin() + dmaBuf1.size(), dmaBuf5.begin() + dmaBuf1.size() + dmaBuf2.size()));
  EXPECT_EQ(dmaBuf3, std::vector<uint8_t>(dmaBuf5.begin() + dmaBuf1.size() + dmaBuf2.size(),
                                          dmaBuf5.begin() + dmaBuf1.size() + dmaBuf2.size() + dmaBuf3.size()));
  EXPECT_EQ(dmaBuf4,
            std::vector<uint8_t>(dmaBuf5.begin() + dmaBuf1.size() + dmaBuf2.size() + dmaBuf3.size(), dmaBuf5.end()));
}

/**********************************************************
 *                                                         *
 *             DMA Stress Testing Functions                *
 *                                                         *
 **********************************************************/
void TestDevOpsApiDmaCmds::dataWRStressSize_2_1(uint8_t maxExp2) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0x91);

  std::vector<std::vector<uint8_t>> writtenBuffs;
  std::vector<std::vector<uint8_t>> readBuffs;
  std::vector<uint8_t> dmaRdBuf;
  std::vector<uint8_t> dmaWrBuf;

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
    readBuffs.push_back(std::move(dmaRdBuf));
  }

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeSync();

  for (std::size_t i = 0; i < writtenBuffs.size(); ++i) {
    EXPECT_EQ(writtenBuffs[i], readBuffs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataWRStressSpeed_2_2(uint8_t maxExp2) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0x101);

  std::vector<std::vector<uint8_t>> writtenBuffs;
  std::vector<std::vector<uint8_t>> readBuffs;
  std::vector<uint8_t> dmaRdBuf;
  std::vector<uint8_t> dmaWrBuf;

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
    readBuffs.push_back(std::move(dmaRdBuf));
    isFirstCommand = false;
  }

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  for (std::size_t i = 0; i < writtenBuffs.size(); ++i) {
    EXPECT_EQ(writtenBuffs[i], readBuffs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataWRStressChannels_2_3() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0x111);

  const uint64_t bufSize = 4 * 1024; // 4K buffer size
  std::vector<uint8_t> dmaWrBuf(bufSize, 0);
  std::vector<std::vector<uint8_t>> readBuffs;
  for (std::size_t i = 0; i < dmaWrBuf.size(); ++i) {
    dmaWrBuf[i] = i % 0x100;
  }

  for (int i = 1; i <= 1000; i++) {
    auto devPhysAddr = getDmaWriteAddr(dmaWrBuf.size());
    auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaWrBuf.data());
    auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devPhysAddr, hostVirtAddr, hostPhysAddr,
                                                       dmaWrBuf.size(),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
  }

  bool isFirstCommand = true; // Barrier for the first command
  for (int i = 1; i <= 1000; i++) {
    std::vector<uint8_t> dmaRdBuf(bufSize, 0);
    auto devPhysAddr = getDmaReadAddr(dmaRdBuf.size());
    auto hostVirtAddr = reinterpret_cast<uint64_t>(dmaRdBuf.data());
    auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), isFirstCommand, devPhysAddr, hostVirtAddr,
                                                      hostPhysAddr, dmaRdBuf.size(),
                                                      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    readBuffs.push_back(std::move(dmaRdBuf));
    isFirstCommand = false;
  }

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  for (std::size_t i = 0; i < readBuffs.size(); ++i) {
    EXPECT_EQ(dmaWrBuf, readBuffs[i]);
  }
}
