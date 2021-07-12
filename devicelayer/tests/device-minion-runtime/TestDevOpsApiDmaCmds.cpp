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
#include <algorithm>
#include <fcntl.h>
#include <random>

namespace {

const uint8_t kNodeCount = 4;

void inline fillRandData(uint8_t* buf, size_t size) {
  auto randFd = open("/dev/urandom", O_RDONLY);
  // Randomize write buffer
  read(randFd, buf, size);
  close(randFd);
}

} // namespace

using namespace dev::dl_tests;

bool TestDevOpsApiDmaCmds::fillDmaStream(int deviceIdx, std::vector<CmdTag>& stream,
                                         const std::vector<std::pair<DmaType, size_t>>& cmdSequence,
                                         std::vector<std::vector<uint8_t>>& dmaWrBufs,
                                         std::vector<std::vector<uint8_t>>& dmaRdBufs) {
  std::vector<uint8_t> dmaWrBuf;
  std::vector<uint8_t> dmaRdBuf;
  bool isFirstRead = true;
  for (int cmdIdx = 0; cmdIdx < cmdSequence.size(); cmdIdx++) {
    switch (cmdSequence[cmdIdx].first) {
    case DmaType::DMA_WRITE:
      dmaWrBuf.resize(cmdSequence[cmdIdx].second, 0);
      fillRandData(dmaWrBuf.data(), dmaWrBuf.size());
      dmaWrBufs.push_back(std::move(dmaWrBuf));
      stream.push_back(IDevOpsApiCmd::createCmd<DataWriteCmd>(
        device_ops_api::CMD_FLAGS_BARRIER_DISABLE, getDmaWriteAddr(deviceIdx, dmaWrBufs.back().size()),
        templ::bit_cast<uint64_t>(dmaWrBufs.back().data()), dmaWrBufs.back().size(),
        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      isFirstRead = true;
      break;
    case DmaType::DMA_READ:
      dmaRdBuf.resize(cmdSequence[cmdIdx].second, 0);
      dmaRdBufs.push_back(std::move(dmaRdBuf));
      stream.push_back(IDevOpsApiCmd::createCmd<DataReadCmd>(
        isFirstRead ? device_ops_api::CMD_FLAGS_BARRIER_ENABLE : device_ops_api::CMD_FLAGS_BARRIER_DISABLE,
        getDmaReadAddr(deviceIdx, dmaRdBufs.back().size()), templ::bit_cast<uint64_t>(dmaRdBufs.back().data()),
        dmaRdBufs.back().size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      isFirstRead = false;
      break;
    default:
      return false;
    }
  }
  return true;
}

bool TestDevOpsApiDmaCmds::executeDmaCmds(bool singleDevice, bool singleQueue, bool isAsync,
                                          const std::vector<std::pair<DmaType, size_t>>& cmdSequence,
                                          std::vector<std::vector<uint8_t>>& dmaWrBufs,
                                          std::vector<std::vector<uint8_t>>& dmaRdBufs) {
  int deviceCount = singleDevice ? 1 : getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<CmdTag> stream;
    int queueCount = singleQueue ? 1 : getSqCount(deviceIdx);
    for (int queueIdx = 0; queueIdx < queueCount; ++queueIdx) {
      if (!fillDmaStream(deviceIdx, stream, cmdSequence, dmaWrBufs, dmaRdBufs)) {
        return false;
      }
      // Save stream against deviceIdx and queueIdx
      insertStream(deviceIdx, queueIdx, std::move(stream));
      stream.clear();
    }
  }

  execute(isAsync);

  deleteStreams();
  return true;
}

bool TestDevOpsApiDmaCmds::allocDmaBufferSequence(bool singleDevice, bool singleQueue,
                                                  const std::vector<std::pair<DmaType, size_t>>& dmaMoveSequence,
                                                  std::unordered_map<size_t, std::vector<uint8_t*>>& dmaWrBufs,
                                                  std::unordered_map<size_t, std::vector<uint8_t*>>& dmaRdBufs) {
  std::vector<std::pair<DmaType, size_t>> wrSequence(dmaMoveSequence.size());
  std::vector<std::pair<DmaType, size_t>> rdSequence(dmaMoveSequence.size());

  auto it = std::copy_if(dmaMoveSequence.begin(), dmaMoveSequence.end(), wrSequence.begin(),
                         [](auto& e) { return e.first == DmaType::DMA_WRITE; });
  wrSequence.resize(std::distance(wrSequence.begin(), it));

  it = std::copy_if(dmaMoveSequence.begin(), dmaMoveSequence.end(), rdSequence.begin(),
                    [](auto& e) { return e.first == DmaType::DMA_READ; });
  rdSequence.resize(std::distance(rdSequence.begin(), it));

  auto totalWrSize =
    std::accumulate(wrSequence.begin(), wrSequence.end(), 0ULL, [](auto& a, auto& b) { return a + b.second; });
  auto totalRdSize =
    std::accumulate(rdSequence.begin(), rdSequence.end(), 0ULL, [](auto& a, auto& b) { return a + b.second; });

  uint8_t* wrMemPtr;
  uint8_t* rdMemPtr;
  auto deviceCount = singleDevice ? 1 : getDevicesCount();
  for (auto deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto queueCount = singleQueue ? 1 : getSqCount(deviceIdx);
    wrMemPtr = static_cast<uint8_t*>(allocDmaBuffer(deviceIdx, queueCount * totalWrSize, true));
    fillRandData(wrMemPtr, queueCount * totalWrSize);
    rdMemPtr = static_cast<uint8_t*>(allocDmaBuffer(deviceIdx, queueCount * totalRdSize, false));
    for (auto queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      std::vector<uint8_t*> dmaWrMemPtrs(wrSequence.size());
      dmaWrBufs.try_emplace(key(deviceIdx, queueIdx), std::move(dmaWrMemPtrs));
      for (size_t i = 0; i < wrSequence.size(); i++) {
        dmaWrBufs[key(deviceIdx, queueIdx)][i] = wrMemPtr;
        wrMemPtr += wrSequence[i].second;
      }
      std::vector<uint8_t*> dmaRdMemPtrs(rdSequence.size());
      dmaRdBufs.try_emplace(key(deviceIdx, queueIdx), std::move(dmaRdMemPtrs));
      for (size_t i = 0; i < rdSequence.size(); i++) {
        dmaRdBufs[key(deviceIdx, queueIdx)][i] = rdMemPtr;
        rdMemPtr += rdSequence[i].second;
      }
    }
  }

  return true;
}

bool TestDevOpsApiDmaCmds::validateAndDeallocDmaBufferSequence(
  bool singleDevice, bool singleQueue, bool validateData,
  const std::vector<std::pair<DmaType, size_t>>& dmaMoveSequence,
  std::unordered_map<size_t, std::vector<uint8_t*>>& dmaWrBufs,
  std::unordered_map<size_t, std::vector<uint8_t*>>& dmaRdBufs) {
  bool res = true;

  auto bytesToCmpPerQueuePerDevice =
    std::min(std::accumulate(dmaMoveSequence.begin(), dmaMoveSequence.end(), 0ULL,
                             [](auto& a, auto& b) { return b.first == DmaType::DMA_WRITE ? a + b.second : a; }),
             std::accumulate(dmaMoveSequence.begin(), dmaMoveSequence.end(), 0ULL,
                             [](auto& a, auto& b) { return b.first == DmaType::DMA_READ ? a + b.second : a; }));

  auto deviceCount = singleDevice ? 1 : getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto queueCount = singleQueue ? 1 : getSqCount(deviceIdx);
    auto bytesToCmpPerDevice = queueCount * bytesToCmpPerQueuePerDevice;
    if (dmaWrBufs.find(key(deviceIdx, 0)) != dmaWrBufs.end() && dmaWrBufs[key(deviceIdx, 0)].size() != 0 &&
        dmaRdBufs.find(key(deviceIdx, 0)) != dmaRdBufs.end() && dmaRdBufs[key(deviceIdx, 0)].size() != 0) {
      if (validateData &&
          memcmp(dmaWrBufs[key(deviceIdx, 0)][0], dmaRdBufs[key(deviceIdx, 0)][0], bytesToCmpPerDevice) != 0) {
        TEST_VLOG(0) << "dmaWrBufs and dmaRdBufs data for device: " << deviceIdx << " did not match!";
        res = false;
      }
      freeDmaBuffer(static_cast<void*>(dmaWrBufs[key(deviceIdx, 0)][0]));
      freeDmaBuffer(static_cast<void*>(dmaRdBufs[key(deviceIdx, 0)][0]));
    } else {
      TEST_VLOG(0) << "dmaWrBufs/dmaRdBufs for device: " << deviceIdx
                   << " does not contain entries conforming to dmaMoveSequence!";
      res = false;
    }
  }
  dmaWrBufs.clear();
  dmaRdBufs.clear();
  return res;
}

bool TestDevOpsApiDmaCmds::fillDmaListStream(int deviceIdx, int queueIdx, std::vector<CmdTag>& stream,
                                             const std::vector<std::pair<DmaType, size_t>>& dmaMoveSequence,
                                             std::unordered_map<size_t, std::vector<uint8_t*>>& dmaWrBufs,
                                             std::unordered_map<size_t, std::vector<uint8_t*>>& dmaRdBufs) {
  if (dmaWrBufs.find(key(deviceIdx, queueIdx)) == dmaWrBufs.end() ||
      dmaWrBufs[key(deviceIdx, queueIdx)].size() !=
        std::count_if(dmaMoveSequence.begin(), dmaMoveSequence.end(),
                      [](auto& e) { return e.first == DmaType::DMA_WRITE; }) ||
      dmaRdBufs.find(key(deviceIdx, queueIdx)) == dmaRdBufs.end() ||
      dmaRdBufs[key(deviceIdx, queueIdx)].size() !=
        std::count_if(dmaMoveSequence.begin(), dmaMoveSequence.end(),
                      [](auto& e) { return e.first == DmaType::DMA_READ; })) {
    return false;
  }

  bool isFirstRead = true;
  int wrNodesFilled = 0;
  int rdNodesFilled = 0;
  int wrIdx = 0;
  int rdIdx = 0;
  std::array<device_ops_api::dma_write_node, kNodeCount> wrList;
  std::array<device_ops_api::dma_read_node, kNodeCount> rdList;
  for (int i = 0; i < dmaMoveSequence.size(); i++) {
    switch (dmaMoveSequence[i].first) {
    case DmaType::DMA_WRITE:
      wrList[wrNodesFilled] = {
        .src_host_virt_addr = templ::bit_cast<uint64_t>(dmaWrBufs[key(deviceIdx, queueIdx)][wrIdx]),
        .src_host_phy_addr = 0,
        .dst_device_phy_addr = getDmaWriteAddr(deviceIdx, dmaMoveSequence[i].second),
        .size = static_cast<uint32_t>(dmaMoveSequence[i].second)};
      wrNodesFilled++;
      wrIdx++;
      break;
    case DmaType::DMA_READ:
      rdList[rdNodesFilled] = {
        .dst_host_virt_addr = templ::bit_cast<uint64_t>(dmaRdBufs[key(deviceIdx, queueIdx)][rdIdx]),
        .dst_host_phy_addr = 0,
        .src_device_phy_addr = getDmaReadAddr(deviceIdx, dmaMoveSequence[i].second),
        .size = static_cast<uint32_t>(dmaMoveSequence[i].second)};
      rdNodesFilled++;
      rdIdx++;
      break;
    default:
      return false;
    }
    if (dmaMoveSequence[i].first == DmaType::DMA_WRITE &&
          (i + 1 == dmaMoveSequence.size() || wrNodesFilled == kNodeCount) ||
        wrNodesFilled > 0 && rdNodesFilled > 0 && dmaMoveSequence[i].first == DmaType::DMA_READ) {
      stream.push_back(IDevOpsApiCmd::createCmd<DmaWriteListCmd>(device_ops_api::CMD_FLAGS_BARRIER_DISABLE,
                                                                 wrList.data(), wrNodesFilled,
                                                                 device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      wrNodesFilled = 0;
      isFirstRead = true;
    }
    if (dmaMoveSequence[i].first == DmaType::DMA_READ &&
          (i + 1 == dmaMoveSequence.size() || rdNodesFilled == kNodeCount) ||
        wrNodesFilled > 0 && rdNodesFilled > 0 && dmaMoveSequence[i].first == DmaType::DMA_WRITE) {
      stream.push_back(IDevOpsApiCmd::createCmd<DmaReadListCmd>(
        isFirstRead ? device_ops_api::CMD_FLAGS_BARRIER_ENABLE : device_ops_api::CMD_FLAGS_BARRIER_DISABLE,
        rdList.data(), rdNodesFilled, device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      rdNodesFilled = 0;
      isFirstRead = false;
    }
  }

  return true;
}

bool TestDevOpsApiDmaCmds::executeDmaListCmds(bool singleDevice, bool singleQueue, bool isAsync,
                                              const std::vector<std::pair<DmaType, size_t>>& dmaMoveSequence) {
  std::unordered_map<size_t, std::vector<uint8_t*>> dmaWrBufs;
  std::unordered_map<size_t, std::vector<uint8_t*>> dmaRdBufs;
  auto res = allocDmaBufferSequence(singleDevice, singleQueue, dmaMoveSequence, dmaWrBufs, dmaRdBufs);
  if (!res) {
    TEST_VLOG(0) << "allocDmaBufferSequence() failed!";
    return res;
  }

  auto deviceCount = singleDevice ? 1 : getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto queueCount = singleQueue ? 1 : getSqCount(deviceIdx);
    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      std::vector<CmdTag> stream;
      if (!fillDmaListStream(deviceIdx, queueIdx, stream, dmaMoveSequence, dmaWrBufs, dmaRdBufs)) {
        TEST_VLOG(0) << "fillDmaListStream() failed for device:" << deviceIdx << ", queue: " << queueIdx;
        validateAndDeallocDmaBufferSequence(singleDevice, singleQueue, false /* only dealloc data */, dmaMoveSequence,
                                            dmaWrBufs, dmaRdBufs);
        deleteStreams();
        return false;
      }
      // Save stream against deviceIdx and queueIdx
      insertStream(deviceIdx, queueIdx, std::move(stream));
      stream.clear();
    }
  }

  execute(isAsync);

  auto validateData = !FLAGS_loopback_driver;
  res =
    validateAndDeallocDmaBufferSequence(singleDevice, singleQueue, validateData, dmaMoveSequence, dmaWrBufs, dmaRdBufs);
  if (!res) {
    TEST_VLOG(0) << "validateAndDeallocDmaBufferSequence() failed!";
  }

  deleteStreams();
  return res;
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
  std::vector<std::pair<DmaType, size_t>> cmdSequence = {{DmaType::DMA_WRITE, 512}, {DmaType::DMA_WRITE, 512},
                                                         {DmaType::DMA_READ, 512},  {DmaType::DMA_READ, 512},
                                                         {DmaType::DMA_WRITE, 256}, {DmaType::DMA_READ, 256}};

  EXPECT_TRUE(executeDmaCmds(false /* multiple devices */, false /* multiple queues */, true /* execute async */,
                             cmdSequence, dmaWrBufs, dmaRdBufs));

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // For DMA received Data Validation
  for (std::size_t i = 0; i < dmaWrBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataRWListCmdMixed() {
  bool singleDevice = false;
  bool singleQueue = false;
  std::vector<std::pair<DmaType, size_t>> dmaMoveSequence = {
    {DmaType::DMA_WRITE, 512}, {DmaType::DMA_WRITE, 256}, {DmaType::DMA_READ, 512}, {DmaType::DMA_READ, 256}};

  EXPECT_TRUE(executeDmaListCmds(singleDevice, singleQueue, true /* execute async */, dmaMoveSequence));
  dmaMoveSequence.clear();
}

// Add variable sized to each transaction, you can pick from
// an array of 5-10 different sizes randomly, and assign to
// the different channels different sizes
void TestDevOpsApiDmaCmds::dataRWCmdMixedWithVarSize_3_6() {
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  std::vector<std::pair<DmaType, size_t>> cmdSequence;
  static const std::array<uint64_t, 10> bufSizeArray = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
  const int kMaxLoopbackCount = 3;
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<> dis(0, bufSizeArray.size() - 1);

  for (int i = 0; i < kMaxLoopbackCount; i++) {
    auto bufSize = bufSizeArray[dis(gen)];
    TEST_VLOG(1) << "Chosen buffer size is: " << bufSize;
    cmdSequence.push_back({DmaType::DMA_WRITE, bufSize});
    cmdSequence.push_back({DmaType::DMA_READ, bufSize});
  }

  EXPECT_TRUE(executeDmaCmds(false /* multiple devices */, false /* multiple queues */, true /* execute async */,
                             cmdSequence, dmaWrBufs, dmaRdBufs));

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // For DMA received Data Validation
  for (std::size_t i = 0; i < dmaWrBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataRWListCmdMixedWithVarSize() {
  bool singleDevice = false;
  bool singleQueue = false;
  std::vector<std::pair<DmaType, size_t>> dmaMoveSequence;
  static const std::array<uint64_t, 10> bufSizeArray = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
  const int kMaxLoopbackCount = 3;
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<> dis(0, bufSizeArray.size() - 1);

  for (int i = 0; i < kMaxLoopbackCount; i++) {
    auto bufSize = bufSizeArray[dis(gen)];
    TEST_VLOG(1) << "Chosen buffer size is: " << bufSize;
    dmaMoveSequence.push_back({DmaType::DMA_WRITE, bufSize});
    dmaMoveSequence.push_back({DmaType::DMA_READ, bufSize});
  }

  EXPECT_TRUE(executeDmaListCmds(singleDevice, singleQueue, true /* execute async */, dmaMoveSequence));
  dmaMoveSequence.clear();
}

// Add minimal 8 DMA transactions with each channel
// having different transfer size, with at least
// 1 CMD in the middle having a Barrier bit set
void TestDevOpsApiDmaCmds::dataRWCmdAllChannels_3_7() {
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  std::vector<std::pair<DmaType, size_t>> cmdSequence;
  const int kMaxLoopbackCount = 4;

  for (int i = 0; i < kMaxLoopbackCount; i++) {
    auto bufSize = 256 * (i + 1);
    cmdSequence.push_back({DmaType::DMA_WRITE, bufSize});
    cmdSequence.push_back({DmaType::DMA_READ, bufSize});
  }

  EXPECT_TRUE(executeDmaCmds(false /* multiple devices */, false /* multiple queues */, true /* execute async */,
                             cmdSequence, dmaWrBufs, dmaRdBufs));

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // For DMA received Data Validation
  for (std::size_t i = 0; i < dmaWrBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataRWListCmdAllChannels() {
  bool singleDevice = false;
  bool singleQueue = false;
  std::vector<std::pair<DmaType, size_t>> dmaMoveSequence;
  const int kMaxLoopbackCount = 4;

  for (int i = 0; i < kMaxLoopbackCount; i++) {
    auto bufSize = 256 * (i + 1);
    dmaMoveSequence.push_back({DmaType::DMA_WRITE, bufSize});
    dmaMoveSequence.push_back({DmaType::DMA_READ, bufSize});
  }

  EXPECT_TRUE(executeDmaListCmds(singleDevice, singleQueue, true /* execute async */, dmaMoveSequence));
  dmaMoveSequence.clear();
}

/**********************************************************
 *                                                         *
 *             DMA Positive Testing Functions              *
 *                                                         *
 **********************************************************/
void TestDevOpsApiDmaCmds::dataRWCmd_PositiveTesting_3_1() {
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  std::vector<std::pair<DmaType, size_t>> cmdSequence = {{DmaType::DMA_WRITE, 64}, {DmaType::DMA_READ, 64}};

  EXPECT_TRUE(executeDmaCmds(false /* multiple devices */, false /* multiple queues */, true /* execute async */,
                             cmdSequence, dmaWrBufs, dmaRdBufs));

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
  std::vector<std::pair<DmaType, size_t>> cmdSequence = {{DmaType::DMA_WRITE, 512},
                                                         {DmaType::DMA_WRITE, 512},
                                                         {DmaType::DMA_WRITE, 512},
                                                         {DmaType::DMA_WRITE, 512},
                                                         {DmaType::DMA_READ, 4 * 512}};

  EXPECT_TRUE(executeDmaCmds(false /* multiple devices */, false /* multiple queues */, true /* execute async */,
                             cmdSequence, dmaWrBufs, dmaRdBufs));

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

void TestDevOpsApiDmaCmds::dataRWListCmdWithBarrier_PositiveTesting() {
  bool singleDevice = false;
  bool singleQueue = false;
  std::vector<std::pair<DmaType, size_t>> dmaMoveSequence = {{DmaType::DMA_WRITE, 512},
                                                             {DmaType::DMA_WRITE, 512},
                                                             {DmaType::DMA_WRITE, 512},
                                                             {DmaType::DMA_WRITE, 512},
                                                             {DmaType::DMA_READ, 4 * 512}};
  EXPECT_TRUE(executeDmaListCmds(singleDevice, singleQueue, true /* execute async */, dmaMoveSequence));
  dmaMoveSequence.clear();
}

void TestDevOpsApiDmaCmds::dataRWListCmd_PositiveTesting_3_11() {
  bool singleDevice = false;
  bool singleQueue = false;
  std::vector<std::pair<DmaType, size_t>> dmaMoveSequence = {
    {DmaType::DMA_WRITE, 64}, {DmaType::DMA_WRITE, 64}, {DmaType::DMA_WRITE, 64}, {DmaType::DMA_WRITE, 64},
    {DmaType::DMA_READ, 64},  {DmaType::DMA_READ, 64},  {DmaType::DMA_READ, 64},  {DmaType::DMA_READ, 64}};

  EXPECT_TRUE(executeDmaListCmds(singleDevice, singleQueue, true /* execute async */, dmaMoveSequence));
  dmaMoveSequence.clear();
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
  std::vector<device_ops_api::dma_write_node> wrList(kNodeCount);
  std::vector<device_ops_api::dma_read_node> rdList(kNodeCount);

  int deviceCount = getDevicesCount();
  int queueIdx = 0;
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<CmdTag> stream;

    dmaWrMemPtrs.push_back(allocDmaBuffer(deviceIdx, kNodeCount * kBufSize, true));
    dmaRdMemPtrs.push_back(allocDmaBuffer(deviceIdx, kNodeCount * kBufSize, false));

    fillRandData(static_cast<uint8_t*>(dmaWrMemPtrs[deviceIdx]), kNodeCount * kBufSize);

    auto* rdBufPtr = static_cast<uint8_t*>(dmaRdMemPtrs[deviceIdx]);
    auto* wrBufPtr = static_cast<uint8_t*>(dmaWrMemPtrs[deviceIdx]);

    for (int nodeIdx = 0; nodeIdx < kNodeCount; ++nodeIdx) {
      wrList[nodeIdx] = {.src_host_virt_addr = templ::bit_cast<uint64_t>(wrBufPtr),
                         .src_host_phy_addr = 0,
                         .dst_device_phy_addr = (nodeIdx == 0) ? 0 : getDmaWriteAddr(deviceIdx, kBufSize),
                         .size = kBufSize};
      wrBufPtr += kBufSize;

      rdList[nodeIdx] = {.dst_host_virt_addr = templ::bit_cast<uint64_t>(rdBufPtr),
                         .dst_host_phy_addr = 0,
                         .src_device_phy_addr = (nodeIdx == 0) ? 0 : getDmaReadAddr(deviceIdx, kBufSize),
                         .size = kBufSize};
      rdBufPtr += kBufSize;
    }
    stream.push_back(
      IDevOpsApiCmd::createCmd<DmaWriteListCmd>(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, wrList.data(), kNodeCount,
                                                device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_INVALID_ADDRESS));
    stream.push_back(
      IDevOpsApiCmd::createCmd<DmaReadListCmd>(device_ops_api::CMD_FLAGS_BARRIER_ENABLE, rdList.data(), kNodeCount,
                                               device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_INVALID_ADDRESS));
    // Save stream against deviceIdx and queueIdx
    insertStream(deviceIdx, queueIdx, std::move(stream));
    stream.clear();
  }
  execute(true);
  deleteStreams();
}

/**********************************************************
 *                                                         *
 *             DMA Stress Testing Functions                *
 *                                                         *
 **********************************************************/
void TestDevOpsApiDmaCmds::dataWRStressSize_2_1(uint8_t maxExp2) {
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  std::vector<std::pair<DmaType, size_t>> cmdSequence;

  for (uint8_t n = 2; n <= maxExp2; n++) {
    cmdSequence.push_back({DmaType::DMA_WRITE, 1ULL << n});
  }

  for (uint8_t n = 2; n <= maxExp2; n++) {
    cmdSequence.push_back({DmaType::DMA_READ, 1ULL << n});
  }

  EXPECT_TRUE(executeDmaCmds(true /* single queue */, true /* single device */, false /* execute sync */, cmdSequence,
                             dmaWrBufs, dmaRdBufs));

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  for (std::size_t i = 0; i < dmaWrBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataWRListStressSize(uint8_t maxExp2) {
  bool singleDevice = true;
  bool singleQueue = true;
  std::vector<std::pair<DmaType, size_t>> dmaMoveSequence;

  for (uint8_t n = 2; n <= maxExp2; n++) {
    dmaMoveSequence.push_back({DmaType::DMA_WRITE, 1ULL << n});
  }

  for (uint8_t n = 2; n <= maxExp2; n++) {
    dmaMoveSequence.push_back({DmaType::DMA_READ, 1ULL << n});
  }

  EXPECT_TRUE(executeDmaListCmds(singleDevice, singleQueue, false /* execute sync */, dmaMoveSequence));
  dmaMoveSequence.clear();
}

void TestDevOpsApiDmaCmds::dataWRStressSpeed_2_2(uint8_t maxExp2) {
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  std::vector<std::pair<DmaType, size_t>> cmdSequence;

  for (uint8_t n = 2; n <= maxExp2; n++) {
    cmdSequence.push_back({DmaType::DMA_WRITE, 1ULL << n});
  }

  for (uint8_t n = 2; n <= maxExp2; n++) {
    cmdSequence.push_back({DmaType::DMA_READ, 1ULL << n});
  }

  EXPECT_TRUE(executeDmaCmds(true /* single queue */, true /* single device */, true /* execute Async */, cmdSequence,
                             dmaWrBufs, dmaRdBufs));

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  for (std::size_t i = 0; i < dmaWrBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dataWRListStressSpeed(uint8_t maxExp2) {
  bool singleDevice = true;
  bool singleQueue = true;
  std::vector<std::pair<DmaType, size_t>> dmaMoveSequence;

  for (uint8_t n = 2; n <= maxExp2; n++) {
    dmaMoveSequence.push_back({DmaType::DMA_WRITE, 1ULL << n});
  }

  for (uint8_t n = 2; n <= maxExp2; n++) {
    dmaMoveSequence.push_back({DmaType::DMA_READ, 1ULL << n});
  }

  EXPECT_TRUE(executeDmaListCmds(singleDevice, singleQueue, true /* execute async */, dmaMoveSequence));
  dmaMoveSequence.clear();
}

void TestDevOpsApiDmaCmds::dataWRStressChannels(bool singleDevice, bool singleQueue, uint32_t numOfLoopbackCmds) {
  std::vector<std::vector<uint8_t>> dmaRdBufs;
  std::vector<std::vector<uint8_t>> dmaWrBufs;
  std::vector<std::pair<DmaType, size_t>> cmdSequence;
  auto deviceCount = singleDevice ? 1 : getDevicesCount();
  auto queueCount = singleQueue ? 1 : getSqCount(0);
  for (int i = 1; i <= (numOfLoopbackCmds / deviceCount) / queueCount; i++) {
    cmdSequence.push_back({DmaType::DMA_WRITE, 4096 /* 4K */});
  }
  for (int i = 1; i <= (numOfLoopbackCmds / deviceCount) / queueCount; i++) {
    cmdSequence.push_back({DmaType::DMA_READ, 4096 /* 4K */});
  }

  EXPECT_TRUE(executeDmaCmds(singleDevice, singleQueue, true /* execute Async */, cmdSequence, dmaWrBufs, dmaRdBufs));
  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  for (std::size_t i = 0; i < dmaRdBufs.size(); ++i) {
    EXPECT_EQ(dmaWrBufs[i], dmaRdBufs[i]);
  }
}

void TestDevOpsApiDmaCmds::dmaListWrAndRd(bool singleDevice, bool singleQueue, uint32_t numOfDmaEntries) {
  std::vector<std::pair<DmaType, size_t>> dmaMoveSequence;
  auto deviceCount = singleDevice ? 1 : getDevicesCount();
  auto queueCount = singleQueue ? 1 : getSqCount(0);
  for (int i = 1; i <= (numOfDmaEntries / deviceCount) / queueCount; i++) {
    dmaMoveSequence.push_back({DmaType::DMA_WRITE, 4096 /* 4K */});
  }
  for (int i = 1; i <= (numOfDmaEntries / deviceCount) / queueCount; i++) {
    dmaMoveSequence.push_back({DmaType::DMA_READ, 4096 /* 4K */});
  }

  EXPECT_TRUE(executeDmaListCmds(singleDevice, singleQueue, true /* execute async */, dmaMoveSequence));
  dmaMoveSequence.clear();
}
