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
#include <random>

namespace {

void inline fillRandData(uint8_t* buf, size_t size) {
  auto randFd = open("/dev/urandom", O_RDONLY);
  // Randomize write buffer
  read(randFd, buf, size);
  close(randFd);
}

} // namespace

bool TestDevOpsApiDmaCmds::fillDmaStream(int deviceIdx, std::vector<std::unique_ptr<IDevOpsApiCmd>>& stream,
                                         const std::vector<DmaType>& cmdSequence,
                                         const std::vector<size_t>& sizeSequence,
                                         std::vector<std::vector<uint8_t>>& dmaWrBufs,
                                         std::vector<std::vector<uint8_t>>& dmaRdBufs) {
  if (!cmdSequence.size() || !sizeSequence.size() || cmdSequence.size() != sizeSequence.size()) {
    return false;
  }
  std::vector<uint8_t> dmaWrBuf;
  std::vector<uint8_t> dmaRdBuf;
  bool isFirstRead = true;
  for (int cmdIdx = 0; cmdIdx < cmdSequence.size(); cmdIdx++) {
    switch (cmdSequence[cmdIdx]) {
    case DmaType::DMA_WRITE:
      dmaWrBuf.resize(sizeSequence[cmdIdx], 0);
      fillRandData(dmaWrBuf.data(), dmaWrBuf.size());
      dmaWrBufs.push_back(std::move(dmaWrBuf));
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(
        device_ops_api::CMD_FLAGS_BARRIER_DISABLE, getDmaWriteAddr(deviceIdx, dmaWrBufs.back().size()),
        templ::bit_cast<uint64_t>(dmaWrBufs.back().data()), templ::bit_cast<uint64_t>(dmaWrBufs.back().data()),
        dmaWrBufs.back().size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      isFirstRead = true;
      break;
    case DmaType::DMA_READ:
      dmaRdBuf.resize(sizeSequence[cmdIdx], 0);
      dmaRdBufs.push_back(std::move(dmaRdBuf));
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(
        isFirstRead ? device_ops_api::CMD_FLAGS_BARRIER_ENABLE : device_ops_api::CMD_FLAGS_BARRIER_DISABLE,
        getDmaReadAddr(deviceIdx, dmaRdBufs.back().size()), templ::bit_cast<uint64_t>(dmaRdBufs.back().data()),
        templ::bit_cast<uint64_t>(dmaRdBufs.back().data()), dmaRdBufs.back().size(),
        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      isFirstRead = false;
      break;
    default:
      return false;
    }
  }
  return true;
}

bool TestDevOpsApiDmaCmds::executeDmaCmds(bool singleDevice, bool singleQueue, bool isAsync,
                                          const std::vector<DmaType>& cmdSequence,
                                          const std::vector<size_t>& sizeSequence,
                                          std::vector<std::vector<uint8_t>>& dmaWrBufs,
                                          std::vector<std::vector<uint8_t>>& dmaRdBufs) {
  int deviceCount = singleDevice ? 1 : getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
    int queueCount = singleQueue ? 1 : getSqCount(deviceIdx);
    for (int queueIdx = 0; queueIdx < queueCount; ++queueIdx) {
      if (!fillDmaStream(deviceIdx, stream, cmdSequence, sizeSequence, dmaWrBufs, dmaRdBufs)) {
        return false;
      }
      // Move stream of commands to streams_
      streams_.try_emplace(key(deviceIdx, queueIdx), std::move(stream));
    }
  }

  isAsync ? executeAsync() : executeSync();

  return true;
}

/**********************************************************
 *                                                         *
 *              DMA Basic Testing Functions                *
 *                                                         *
 **********************************************************/
// Read/Write access mixed b2b .. i.e. DMA WRITE, DMA READ,
// DMA READ, DMA WRITE. The address would be orthogonal to each other.
void TestDevOpsApiDmaCmds::dataRWCmdMixed_3_5() {
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  std::vector<DmaType> cmdSequence = {DmaType::DMA_WRITE, DmaType::DMA_WRITE, DmaType::DMA_READ,
                                      DmaType::DMA_READ,  DmaType::DMA_WRITE, DmaType::DMA_READ};
  std::vector<size_t> sizeSequence = {512, 512, 512, 512, 256, 256};

  EXPECT_TRUE(executeDmaCmds(false /* multiple devices */, false /* multiple queues */, true /* execute async */,
                             cmdSequence, sizeSequence, dmaWrBufs, dmaRdBufs));

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
  std::vector<DmaType> cmdSequence;
  std::vector<size_t> sizeSequence;
  static const std::array<uint64_t, 10> bufSizeArray = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
  const int kMaxLoopbackCount = 3;
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<> dis(0, bufSizeArray.size() - 1);

  for (int i = 0; i < kMaxLoopbackCount; i++) {
    auto bufSize = bufSizeArray[dis(gen)];
    TEST_VLOG(1) << "Chosen buffer size is: " << bufSize;
    sizeSequence.push_back(bufSize); // Write buffer size
    cmdSequence.push_back(DmaType::DMA_WRITE);
    sizeSequence.push_back(bufSize); // Read buffer size
    cmdSequence.push_back(DmaType::DMA_READ);
  }

  EXPECT_TRUE(executeDmaCmds(false /* multiple devices */, false /* multiple queues */, true /* execute async */,
                             cmdSequence, sizeSequence, dmaWrBufs, dmaRdBufs));

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
  std::vector<DmaType> cmdSequence;
  std::vector<size_t> sizeSequence;
  const int kMaxLoopbackCount = 4;

  for (int i = 0; i < kMaxLoopbackCount; i++) {
    auto bufSize = 256 * (i + 1);
    sizeSequence.push_back(bufSize); // Write buffer size
    cmdSequence.push_back(DmaType::DMA_WRITE);
    sizeSequence.push_back(bufSize); // Read buffer size
    cmdSequence.push_back(DmaType::DMA_READ);
  }

  EXPECT_TRUE(executeDmaCmds(false /* multiple devices */, false /* multiple queues */, true /* execute async */,
                             cmdSequence, sizeSequence, dmaWrBufs, dmaRdBufs));

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
  std::vector<DmaType> cmdSequence = {DmaType::DMA_WRITE, DmaType::DMA_READ};
  std::vector<size_t> sizeSequence = {64, 64};

  EXPECT_TRUE(executeDmaCmds(false /* multiple devices */, false /* multiple queues */, true /* execute async */,
                             cmdSequence, sizeSequence, dmaWrBufs, dmaRdBufs));

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
  std::vector<DmaType> cmdSequence = {DmaType::DMA_WRITE, DmaType::DMA_WRITE, DmaType::DMA_WRITE, DmaType::DMA_WRITE,
                                      DmaType::DMA_READ};
  std::vector<size_t> sizeSequence = {512, 512, 512, 512, 4 * 512};

  EXPECT_TRUE(executeDmaCmds(false /* multiple devices */, false /* multiple queues */, true /* execute async */,
                             cmdSequence, sizeSequence, dmaWrBufs, dmaRdBufs));

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

  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto queueCount = getSqCount(deviceIdx);
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;

    dmaWrMemPtrs.push_back(allocDmaBuffer(deviceIdx, queueCount * kNodeCount * kBufSize, true));
    dmaRdMemPtrs.push_back(allocDmaBuffer(deviceIdx, queueCount * kNodeCount * kBufSize, false));

    fillRandData(static_cast<uint8_t*>(dmaWrMemPtrs[deviceIdx]), queueCount * kNodeCount * kBufSize);

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
      streams_.try_emplace(key(deviceIdx, queueIdx), std::move(stream));
    }
  }
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

  int deviceCount = getDevicesCount();
  int queueIdx = 0;
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;

    dmaWrMemPtrs.push_back(allocDmaBuffer(deviceIdx, kNodeCount * kBufSize, true));
    dmaRdMemPtrs.push_back(allocDmaBuffer(deviceIdx, kNodeCount * kBufSize, false));

    fillRandData(static_cast<uint8_t*>(dmaWrMemPtrs[deviceIdx]), kNodeCount * kBufSize);

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
    streams_.try_emplace(key(deviceIdx, queueIdx), std::move(stream));
  }
  executeAsync();
}

/**********************************************************
 *                                                         *
 *             DMA Stress Testing Functions                *
 *                                                         *
 **********************************************************/
void TestDevOpsApiDmaCmds::dataWRStressSize_2_1(uint8_t maxExp2) {
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  std::vector<DmaType> cmdSequence;
  std::vector<size_t> sizeSequence;

  for (uint8_t n = 2; n <= maxExp2; n++) {
    cmdSequence.push_back(DmaType::DMA_WRITE);
    sizeSequence.push_back(1ULL << n);
  }

  for (uint8_t n = 2; n <= maxExp2; n++) {
    cmdSequence.push_back(DmaType::DMA_READ);
    sizeSequence.push_back(1ULL << n);
  }

  EXPECT_TRUE(executeDmaCmds(true /* single queue */, true /* single device */, false /* execute sync */, cmdSequence,
                             sizeSequence, dmaWrBufs, dmaRdBufs));

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  for (std::size_t i = 0; i < dmaWrBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataWRStressSpeed_2_2(uint8_t maxExp2) {
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  std::vector<DmaType> cmdSequence;
  std::vector<size_t> sizeSequence;

  for (uint8_t n = 2; n <= maxExp2; n++) {
    cmdSequence.push_back(DmaType::DMA_WRITE);
    sizeSequence.push_back(1ULL << n);
  }

  for (uint8_t n = 2; n <= maxExp2; n++) {
    cmdSequence.push_back(DmaType::DMA_READ);
    sizeSequence.push_back(1ULL << n);
  }

  EXPECT_TRUE(executeDmaCmds(true /* single queue */, true /* single device */, true /* execute Async */, cmdSequence,
                             sizeSequence, dmaWrBufs, dmaRdBufs));

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  for (std::size_t i = 0; i < dmaWrBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataWRStressChannels(bool singleDevice, bool singleQueue, uint32_t numOfLoopbackCmds) {
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<DmaType> cmdSequence;
  std::vector<size_t> sizeSequence;
  int deviceCount = singleDevice ? 1 : getDevicesCount();
  int queueCount = singleQueue ? 1 : getSqCount(0);
  for (int i = 1; i <= (numOfLoopbackCmds / deviceCount) / queueCount; i++) {
    cmdSequence.push_back(DmaType::DMA_WRITE);
    sizeSequence.push_back(4096 /* 4K */);
  }
  for (int i = 1; i <= (numOfLoopbackCmds / deviceCount) / queueCount; i++) {
    cmdSequence.push_back(DmaType::DMA_READ);
    sizeSequence.push_back(4096 /* 4K */);
  }

  EXPECT_TRUE(executeDmaCmds(singleDevice, singleQueue, true /* execute Async */, cmdSequence, sizeSequence, dmaWrBufs,
                             dmaRdBufs));

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  for (std::size_t i = 0; i < dmaRdBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
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

  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;

    dmaWrMemPtrs.push_back(allocDmaBuffer(deviceIdx, numOfDmaEntriesPerDev * kBufSize, true));
    dmaRdMemPtrs.push_back(allocDmaBuffer(deviceIdx, numOfDmaEntriesPerDev * kBufSize, false));

    fillRandData(static_cast<uint8_t*>(dmaWrMemPtrs[deviceIdx]), numOfDmaEntriesPerDev * kBufSize);

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
      streams_.try_emplace(key(deviceIdx, queueIdx), std::move(stream));
    }
  }
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
