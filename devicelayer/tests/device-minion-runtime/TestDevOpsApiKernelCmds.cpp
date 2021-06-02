//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestDevOpsApiKernelCmds.h"
#include "Autogen.h"
#include <bitset>
#include <experimental/filesystem>
#include <random>

using namespace ELFIO;
namespace fs = std::experimental::filesystem;

/**********************************************************
 *                                                         *
 *               Kernel Functional Tests                   *
 *                                                         *
 **********************************************************/
void TestDevOpsApiKernelCmds::launchAddVectorKernel_PositiveTesting_4_1(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  constexpr int maxQueueCount = 8; // upper limit for now
  int deviceIdx = 0;
  auto queueCount = getSqCount(deviceIdx);

  // Load ELF
  std::vector<ELFIO::elfio> reader(queueCount);
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("add_vector.elf")).string();
  std::vector<uint64_t> kernelEntryAddr(queueCount, 0);

  std::minstd_rand simple_rand;
  simple_rand.seed(time(NULL));
  // Create vector data
  std::vector<std::vector<int>> vA, vB, vSum;
  int numElems = 10496;
  for (uint16_t queueId = 0; queueId < queueCount; queueId++) {
    std::vector<int> tempVecA, tempVecB, tempSum;
    for (int i = 0; i < numElems; ++i) {
      tempVecA.emplace_back(simple_rand());
      tempVecB.emplace_back(simple_rand());
      tempSum.emplace_back(tempVecA.back() + tempVecB.back());
    }
    vA.push_back(std::move(tempVecA));
    vB.push_back(std::move(tempVecB));
    vSum.push_back(std::move(tempSum));
  }

  std::vector<std::vector<int>> resultFromDevice;
  std::vector<int> readBuf;

  // Assign device memory for data
  auto bufSize = numElems * sizeof(int);
  auto alignedBufSize = ALIGN(bufSize, kCacheLineSize);
  std::vector<uint64_t> dataLoadAddr(queueCount, 0);
  std::vector<uint64_t> devAddrVecA(queueCount, 0);
  std::vector<uint64_t> devAddrVecB(queueCount, 0);
  std::vector<uint64_t> devAddrVecResult(queueCount, 0);
  std::vector<uint64_t> hostVirtAddrA(queueCount, 0);
  std::vector<uint64_t> hostVirtAddrB(queueCount, 0);
  std::vector<uint64_t> devAddrKernelArgs(queueCount, 0);
  std::vector<uint64_t> devAddrkernelException(queueCount, 0);
  // create device params structure
  struct Params {
    uint64_t vA;
    uint64_t vB;
    uint64_t vResult;
    int numElements;
  };
  std::vector<Params> parameters(queueCount);

  for (int queueId = 0; queueId < queueCount; queueId++) {
    stream.clear();

    dataLoadAddr[queueId] = getDmaWriteAddr(deviceIdx, 3 * alignedBufSize);
    devAddrVecA[queueId] = dataLoadAddr[queueId];
    devAddrVecB[queueId] = devAddrVecA[queueId] + alignedBufSize;
    devAddrVecResult[queueId] = devAddrVecB[queueId] + alignedBufSize;

    parameters[queueId] = {devAddrVecA[queueId], devAddrVecB[queueId], devAddrVecResult[queueId], numElems};
    auto sizeOfParams = sizeof(Params);

    // Load ELF
    loadElfToDevice(deviceIdx, reader[queueId], elfPath, stream, kernelEntryAddr[queueId]);
    // Copy kernel input data to device
    hostVirtAddrA[queueId] = reinterpret_cast<uint64_t>(vA[queueId].data());
    auto hostPhysAddrA = hostVirtAddrA[queueId]; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(false, devAddrVecA[queueId], hostVirtAddrA[queueId],
                                                       hostPhysAddrA, vA[queueId].size() * sizeof(vA[queueId][0]),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    hostVirtAddrB[queueId] = reinterpret_cast<uint64_t>(vB[queueId].data());
    auto hostPhysAddrB = hostVirtAddrB[queueId]; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(false, devAddrVecB[queueId], hostVirtAddrB[queueId],
                                                       hostPhysAddrB, vB[queueId].size() * sizeof(vB[queueId][0]),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

    // allocate space for kernel args
    devAddrKernelArgs[queueId] = getDmaWriteAddr(deviceIdx, 0x400);

    // allocate 1MB space for kernel error/exception buffer
    devAddrkernelException[queueId] = getDmaWriteAddr(deviceIdx, 0x100000);

    // Launch Kernel Command
    stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
      true, kernelEntryAddr[queueId], devAddrKernelArgs[queueId], devAddrkernelException[queueId], shire_mask, 0,
      reinterpret_cast<void*>(&parameters.at(queueId)), sizeOfParams,
      device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));

    // Read back Kernel Results from device
    readBuf.resize(numElems, 0);
    auto hostVirtAddr = reinterpret_cast<uint64_t>(readBuf.data());
    auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(true, devAddrVecResult[queueId], hostVirtAddr, hostPhysAddr,
                                                      readBuf.size() * sizeof(readBuf[0]),
                                                      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    resultFromDevice.push_back(std::move(readBuf));
    // Move stream of commands to streams_
    streams_.emplace(key(deviceIdx, queueId), std::move(stream));
  }

  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // Verify Vector's Data
  for (int queueId = 0; queueId < queueCount; queueId++) {
    ASSERT_EQ(resultFromDevice[queueId], vSum[queueId]);
  }

  TEST_VLOG(1) << "====> ADD TWO VECTORS KERNEL RESPONSE DATA VERIFIED <====" << std::endl;
}

void TestDevOpsApiKernelCmds::launchUberKernel_PositiveTesting_4_4(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  int deviceIdx = 0;
  auto queueCount = getSqCount(deviceIdx);

  // Load ELF
  std::vector<ELFIO::elfio> reader(queueCount);
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("uberkernel.elf")).string();
  std::vector<uint64_t> kernelEntryAddr(queueCount, 0);
  std::vector<uint64_t> devAddrBufLayer0(queueCount, 0);
  std::vector<uint64_t> devAddrBufLayer1(queueCount, 0);
  std::vector<uint64_t> devAddrKernelArgs(queueCount, 0);
  std::vector<uint64_t> devAddrkernelException(queueCount, 0);

  // Allocate device buffer for layer 0
  const uint64_t numElemsLayer0 = 200;
  const uint64_t bufSizeLayer0 = sizeof(uint64_t) * numElemsLayer0;

  // Allocate device buffer for layer 1
  const uint64_t numElemsLayer1 = 400;
  const uint64_t bufSizeLayer1 = sizeof(uint64_t) * numElemsLayer1;

  // Setup and allocate kernel launch args
  struct layer_parameters_t {
    uint64_t data_ptr;
    uint64_t length;
    uint32_t shire_count;
  };

  layer_parameters_t launchArgs[2];
  std::vector<std::vector<uint64_t>> resultFromDevice;
  std::vector<uint64_t> readBuf;
  for (uint8_t queueId = 0; queueId < queueCount; queueId++) {

    devAddrBufLayer0[queueId] = getDmaWriteAddr(deviceIdx, bufSizeLayer0);
    devAddrBufLayer1[queueId] = getDmaWriteAddr(deviceIdx, bufSizeLayer1);
    TEST_VLOG(1) << "SQ : " << queueId << " devAddrBufLayer0: 0x" << std::hex << devAddrBufLayer0[queueId];
    TEST_VLOG(1) << "SQ : " << queueId << " devAddrBufLayer1: 0x" << std::hex << devAddrBufLayer1[queueId];
    devAddrKernelArgs[queueId] = getDmaWriteAddr(deviceIdx, (sizeof(launchArgs), kCacheLineSize));
    // allocate 1MB space for kernel error/exception buffer
    devAddrkernelException[queueId] = getDmaWriteAddr(deviceIdx, 0x100000);

    launchArgs[0].data_ptr = devAddrBufLayer0[queueId];
    launchArgs[0].length = bufSizeLayer0;
    launchArgs[0].shire_count = std::bitset<64>(shire_mask).count();
    launchArgs[1].data_ptr = devAddrBufLayer1[queueId];
    launchArgs[1].length = bufSizeLayer1;
    launchArgs[1].shire_count = std::bitset<64>(shire_mask).count();

    loadElfToDevice(deviceIdx, reader[queueId], elfPath, stream, kernelEntryAddr[queueId]);

    // Kernel launch Command
    stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
      device_ops_api::CMD_FLAGS_BARRIER_ENABLE, kernelEntryAddr[queueId], devAddrKernelArgs[queueId],
      devAddrkernelException[queueId], shire_mask, 0, reinterpret_cast<void*>(launchArgs), sizeof(launchArgs),
      device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));

    // Read back data written by layer 0
    readBuf.resize(numElemsLayer0, 0xEEEEEEEEEEEEEEEEULL);
    auto hostVirtAddr = reinterpret_cast<uint64_t>(readBuf.data());
    auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(
      device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrBufLayer0[queueId], hostVirtAddr, hostPhysAddr,
      readBuf.size() * sizeof(readBuf[0]), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    resultFromDevice.push_back(std::move(readBuf));

    // Read back data written by layer 1
    readBuf.resize(numElemsLayer1, 0xEEEEEEEEEEEEEEEEULL);
    hostVirtAddr = reinterpret_cast<uint64_t>(readBuf.data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(
      device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrBufLayer1[queueId], hostVirtAddr, hostPhysAddr,
      readBuf.size() * sizeof(readBuf[0]), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    resultFromDevice.push_back(std::move(readBuf));

    // Move stream of commands to streams_
    streams_.emplace(key(deviceIdx, queueId), std::move(stream));
  }
  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // Verify data
  const std::vector<uint64_t> refdata0(numElemsLayer0, 0xBEEFBEEFBEEFBEEFULL);
  const std::vector<uint64_t> refdata1(numElemsLayer1, 0xBEEFBEEFBEEFBEEFULL);
  for (uint8_t queueId = 0; queueId < queueCount; queueId++) {
    ASSERT_EQ(resultFromDevice[queueId * 2], refdata0);
    ASSERT_EQ(resultFromDevice[(queueId * 2) + 1], refdata1);
  }

  TEST_VLOG(1) << "====> UBERKERNEL KERNEL RESPONSE DATA VERIFIED <====" << std::endl;
}

void TestDevOpsApiKernelCmds::launchEmptyKernel_PositiveTesting_4_5(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  int deviceIdx = 0;
  auto queueCount = getSqCount(deviceIdx);

  // Read ELF
  std::vector<ELFIO::elfio> reader(queueCount);
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("empty.elf")).string();
  std::vector<uint64_t> kernelEntryAddr(queueCount, 0);
  std::vector<uint64_t> devAddrKernelArgs(queueCount, 0);
  std::vector<uint64_t> devAddrKernelResult(queueCount, 0);
  std::vector<uint64_t> devAddrkernelException(queueCount, 0);

  uint8_t dummyKernelArgs[] = {0xDE, 0xAD, 0xBE, 0xEF};
  std::vector<std::vector<uint8_t>> resultFromDevice;

  for (uint8_t queueId = 0; queueId < queueCount; queueId++) {
    // Load ELF
    loadElfToDevice(deviceIdx, reader[queueId], elfPath, stream, kernelEntryAddr[queueId]);
    // allocate space for kernel args
    devAddrKernelArgs[queueId] = getDmaWriteAddr(deviceIdx, ALIGN(sizeof(dummyKernelArgs), kCacheLineSize));
    // allocate 4KB space for dummy kernel result
    devAddrKernelResult[queueId] = getDmaWriteAddr(deviceIdx, ALIGN(sizeof(0x1000), kCacheLineSize));
    // allocate 1MB space for kernel error/exception buffer
    devAddrkernelException[queueId] = getDmaWriteAddr(deviceIdx, 0x100000);
    // kernel launch
    stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
      device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kernelEntryAddr[queueId], devAddrKernelArgs[queueId],
      devAddrkernelException[queueId], shire_mask, 0, reinterpret_cast<void*>(dummyKernelArgs), sizeof(dummyKernelArgs),
      device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));
    // Do a dummy read back Kernel Results from device
    std::vector<uint8_t> readBuf(0x1000, 0);
    auto hostVirtAddr = reinterpret_cast<uint64_t>(readBuf.data());
    auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(
      device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrKernelResult[queueId], hostVirtAddr, hostPhysAddr,
      readBuf.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    resultFromDevice.push_back(std::move(readBuf));
    // Move stream of commands to streams_
    streams_.emplace(key(deviceIdx, queueId), std::move(stream));
  }
  executeAsync();

  TEST_VLOG(1) << "====> EMPTY KERNEL DONE <====" << std::endl;
}

void TestDevOpsApiKernelCmds::launchExceptionKernel_NegativeTesting_4_6(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  int deviceIdx = 0;
  auto queueCount = getSqCount(deviceIdx);

  // Load ELF
  std::vector<ELFIO::elfio> reader(queueCount);
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("exception.elf")).string();
  std::vector<uint64_t> kernelEntryAddr(queueCount, 0);
  std::vector<uint64_t> devAddrkernelException(queueCount, 0);
  std::vector<std::vector<uint8_t>> resultFromDevice;
  std::vector<device_ops_api::tag_id_t> kernelLaunchTagId;

  for (uint8_t queueId = 0; queueId < queueCount; queueId++) {
    // allocate 1MB space for kernel error/exception buffer
    devAddrkernelException[queueId] = getDmaWriteAddr(deviceIdx, 0x100000);
    loadElfToDevice(deviceIdx, reader[queueId], elfPath, stream, kernelEntryAddr[queueId]);
    // Kernel launch
    auto kernelCmd = IDevOpsApiCmd::createKernelLaunchCmd(
      device_ops_api::CMD_FLAGS_BARRIER_ENABLE, kernelEntryAddr[queueId], 0 /* No kernel args */,
      devAddrkernelException[queueId], shire_mask, 0, NULL, 0,
      device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_EXCEPTION, "exception.elf");
    kernelLaunchTagId.push_back(kernelCmd->getCmdTagId());
    stream.push_back(std::move(kernelCmd));

    // pull the exception buffer from device
    std::vector<uint8_t> readBuf(0x100000, 0);
    auto hostVirtAddr = reinterpret_cast<uint64_t>(readBuf.data());
    auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(
      device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrkernelException[queueId], hostVirtAddr, hostPhysAddr,
      readBuf.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

    resultFromDevice.push_back(std::move(readBuf));
    // Move stream of commands to streams_
    streams_.emplace(key(deviceIdx, queueId), std::move(stream));
  }
  executeAsync();

  // print the exception buffer for the first two shires
  for (int queueId = 0; queueId < queueCount; queueId++) {
    printErrorContext(queueId, reinterpret_cast<void*>(resultFromDevice[queueId].data()), 0x3,
                      kernelLaunchTagId[queueId]);
  }

  TEST_VLOG(1) << "====> EXCEPTION KERNEL DONE <====" << std::endl;
}

void TestDevOpsApiKernelCmds::abortHangKernel_PositiveTesting_4_10(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  int deviceIdx = 0;
  auto queueCount = getSqCount(deviceIdx);

  // Load ELF
  std::vector<ELFIO::elfio> reader(queueCount);
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("hang.elf")).string();
  std::vector<uint64_t> kernelEntryAddr(queueCount, 0);
  std::vector<uint64_t> devAddrkernelException(queueCount, 0);
  std::vector<std::vector<uint8_t>> resultFromDevice;
  std::vector<device_ops_api::tag_id_t> kernelLaunchTagId;

  for (uint8_t queueId = 0; queueId < queueCount; queueId++) {
    // allocate 1MB space for kernel error/exception buffer
    devAddrkernelException[queueId] = getDmaWriteAddr(deviceIdx, 0x100000);
    loadElfToDevice(deviceIdx, reader[queueId], elfPath, stream, kernelEntryAddr[queueId]);
    // Kernel launch
    auto kernelCmd = IDevOpsApiCmd::createKernelLaunchCmd(
      device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kernelEntryAddr[queueId], 0 /* No kernel args */,
      devAddrkernelException[queueId], shire_mask, 0, NULL, 0,
      device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED, "hang.elf");
    kernelLaunchTagId.push_back(kernelCmd->getCmdTagId());
    stream.push_back(std::move(kernelCmd));

    // Kernel Abort
    stream.push_back(IDevOpsApiCmd::createKernelAbortCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE,
                                                         kernelLaunchTagId[queueId],
                                                         device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS));

    // pull the exception buffer from device
    std::vector<uint8_t> readBuf(0x100000, 0);
    auto hostVirtAddr = reinterpret_cast<uint64_t>(readBuf.data());
    auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(
      device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrkernelException[queueId], hostVirtAddr, hostPhysAddr,
      readBuf.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    resultFromDevice.push_back(std::move(readBuf));

    // Move stream of commands to streams_
    streams_.emplace(key(deviceIdx, queueId), std::move(stream));
  }
  executeAsync();

  // print the exception buffer for the first two shires
  for (int queueId = 0; queueId < queueCount; queueId++) {
    printErrorContext(queueId, reinterpret_cast<void*>(resultFromDevice[queueId].data()), 0x3,
                      kernelLaunchTagId[queueId]);
  }

  TEST_VLOG(1) << "====> HANG KERNEL DONE <====" << std::endl;
}

void TestDevOpsApiKernelCmds::kernelAbortCmd_InvalidTagIdNegativeTesting_6_2() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  int deviceIdx = 0;
  auto queueCount = getSqCount(deviceIdx);
  for (uint8_t queueId = 0; queueId < queueCount; queueId++) {
    stream.push_back(
      IDevOpsApiCmd::createKernelAbortCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, 0xbeef /* invalid tagId */,
                                          device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_INVALID_TAG_ID));

    // Move stream of commands to streams_[queueId]
    streams_.emplace(key(deviceIdx, queueId), std::move(stream));
  }
  executeAsync();
}

/**********************************************************
 *                                                         *
 *                  Kernel Stress Tests                    *
 *                                                         *
 **********************************************************/
void TestDevOpsApiKernelCmds::backToBackSameKernelLaunchCmds_3_1(bool singleQueue, uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  int deviceIdx = 0;
  auto queueCount = singleQueue ? 1 : getSqCount(deviceIdx);

  const int totalKer = 100;
  // create device params structure
  struct Params {
    uint64_t vA;
    uint64_t vB;
    uint64_t vResult;
    int numElements;
  } kerParams[totalKer];
  std::vector<ELFIO::elfio> reader(queueCount);
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("add_vector.elf")).string();
  std::vector<uint64_t> kernelEntryAddr(queueCount, 0);
  std::vector<uint64_t> dataLoadAddr(queueCount, 0);
  std::vector<uint64_t> devAddrKernelArgs(queueCount, 0);
  std::vector<uint64_t> devAddrkernelException(queueCount, 0);
  std::vector<uint64_t> hostVirtAddrVecA(queueCount, 0);
  std::vector<uint64_t> hostVirtAddrVecB(queueCount, 0);
  int numElems = 4 * 1024;
  auto bufSize = numElems * sizeof(int);
  auto alignedBufSize = ALIGN(bufSize, kCacheLineSize);
  std::minstd_rand simple_rand;
  simple_rand.seed(time(NULL));
  // Create vector data
  std::vector<std::vector<int>> vA, vB, vSum;
  for (uint16_t queueId = 0; queueId < queueCount; queueId++) {
    std::vector<int> tempVecA, tempVecB, tempSum;
    for (int i = 0; i < numElems; ++i) {
      tempVecA.emplace_back(simple_rand());
      tempVecB.emplace_back(simple_rand());
      tempSum.emplace_back(tempVecA.back() + tempVecB.back());
    }
    vA.push_back(std::move(tempVecA));
    vB.push_back(std::move(tempVecB));
    vSum.push_back(std::move(tempSum));
  }

  uint64_t devAddrVecResult[totalKer];
  std::vector<std::vector<int>> outputVec;
  int paramIndex = 0;
  int readIndex = 0;

  for (uint16_t queueId = 0; queueId < queueCount; queueId++) {
    hostVirtAddrVecA[queueId] = reinterpret_cast<uint64_t>(vA[queueId].data());
    auto hostPhysAddrVecA =
      hostVirtAddrVecA[queueId]; // Should be handled in SysEmu, userspace should not fill this value
    hostVirtAddrVecB[queueId] = reinterpret_cast<uint64_t>(vB[queueId].data());
    auto hostPhysAddrVecB =
      hostVirtAddrVecB[queueId]; // Should be handled in SysEmu, userspace should not fill this value
    // Load kernel ELF
    loadElfToDevice(deviceIdx, reader[queueId], elfPath, stream, kernelEntryAddr[queueId]);
    // Copy kernel input data to device
    dataLoadAddr[queueId] = getDmaWriteAddr(deviceIdx, 2 * alignedBufSize);
    auto devAddrVecA = dataLoadAddr[queueId];
    auto devAddrVecB = devAddrVecA + alignedBufSize;
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(
      device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devAddrVecA, hostVirtAddrVecA[queueId], hostPhysAddrVecA,
      vA[queueId].size() * sizeof(vA[queueId][0]), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(
      device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devAddrVecB, hostVirtAddrVecB[queueId], hostPhysAddrVecB,
      vB[queueId].size() * sizeof(vB[queueId][0]), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    for (int i = 0; i < totalKer / queueCount; i++) {
      // allocate space for kernel args
      devAddrVecResult[paramIndex] = getDmaWriteAddr(deviceIdx, alignedBufSize);
      devAddrKernelArgs[queueId] = getDmaWriteAddr(deviceIdx, ALIGN(sizeof(Params), kCacheLineSize));
      kerParams[paramIndex] = {devAddrVecA, devAddrVecB, devAddrVecResult[paramIndex], numElems};

      // allocate 1MB space for kernel error/exception buffer
      devAddrkernelException[queueId] = getDmaWriteAddr(deviceIdx, 0x100000);

      // Launch Kernel Command
      stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
        device_ops_api::CMD_FLAGS_BARRIER_ENABLE, kernelEntryAddr[queueId], devAddrKernelArgs[queueId],
        devAddrkernelException[queueId], shire_mask, 0, reinterpret_cast<void*>(&kerParams[paramIndex]), sizeof(Params),
        device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));
      paramIndex++;
    }

    // Read back Kernel Results from device
    for (int i = 0; i < totalKer / queueCount; i++) {
      std::vector<int> resultFromDevice(numElems, 0);
      auto hostVirtAddrRes = reinterpret_cast<uint64_t>(resultFromDevice.data());
      auto hostPhysAddrRes = hostVirtAddrRes; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(
        i == 0, /* Barrier only for first read to make sure that all kernels execution done */
        devAddrVecResult[readIndex], hostVirtAddrRes, hostPhysAddrRes,
        resultFromDevice.size() * sizeof(resultFromDevice[0]), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      outputVec.push_back(std::move(resultFromDevice));
      readIndex++;
    }

    // Move stream of commands to streams_
    streams_.emplace(key(deviceIdx, queueId), std::move(stream));
  }

  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // Verify Vector's Data
  readIndex = 0;
  for (uint16_t queueId = 0; queueId < queueCount; queueId++) {
    for (int i = 0; i < totalKer / queueCount; i++) {
      ASSERT_EQ(outputVec[readIndex], vSum[queueId])
        << "outputVec[" << readIndex << "] of SQ " << queueId << " result validation failed!";
      readIndex++;
    }
  }

  TEST_VLOG(1) << "====> BACK TO BACK " << totalKer << " KERNEL LAUNCH (ADD VECTORS KERNEL) DATA VERIFIED <====\n"
               << std::endl;
}

void TestDevOpsApiKernelCmds::backToBackDifferentKernelLaunchCmds_3_2(bool singleQueue, uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  int deviceIdx = 0;
  auto queueCount = singleQueue ? 1 : getSqCount(deviceIdx);
  const int totalKer = 100;
  int totalAddKerPerSq[queueCount] = {0};
  enum kernelTypes { addKerType = 0, excKerType, hangKerType };
  uint8_t kerTypes[totalKer];
  uint8_t totalAddKer = 0;
  srand(time(0));
  for (int i = 0; i < totalKer; i++) {
    kerTypes[i] = rand() % 3;
    if (addKerType == kerTypes[i])
      totalAddKer++;
  }

  /*******************************************************
   *                  load all ELF files                 *
   * *****************************************************/
  // Load add_vector ELF kerTypes = 0
  std::vector<ELFIO::elfio> addKerReader(queueCount);
  auto addKerElfPath = (fs::path(FLAGS_kernels_dir) / fs::path("add_vector.elf")).string();
  // Load exception ELF kerTypes = 1
  std::vector<ELFIO::elfio> excepKerReader(queueCount);
  auto excepKerElfPath = (fs::path(FLAGS_kernels_dir) / fs::path("exception.elf")).string();
  // Load hang ELF kerTypes = 2
  std::vector<ELFIO::elfio> hangKerReader(queueCount);
  auto hangKerElfPath = (fs::path(FLAGS_kernels_dir) / fs::path("hang.elf")).string();

  std::vector<uint64_t> addKernelEntryAddr(queueCount, 0);
  std::vector<uint64_t> excepKerEntryAddr(queueCount, 0);
  std::vector<uint64_t> hangKerEntryAddr(queueCount, 0);

  /*******************************************************
   *                  Setup for Add Kernel               *
   * *****************************************************/
  std::minstd_rand simple_rand;
  simple_rand.seed(time(NULL));
  // Create vector data
  std::vector<std::vector<int>> vA, vB, vSum;
  int numElems = 1024;
  for (uint16_t queueId = 0; queueId < queueCount; queueId++) {
    std::vector<int> tempVecA, tempVecB, tempSum;
    for (int i = 0; i < numElems; ++i) {
      tempVecA.emplace_back(simple_rand());
      tempVecB.emplace_back(simple_rand());
      tempSum.emplace_back(tempVecA.back() + tempVecB.back());
    }
    vA.push_back(std::move(tempVecA));
    vB.push_back(std::move(tempVecB));
    vSum.push_back(std::move(tempSum));
  }
  // create device params structure
  struct Params {
    uint64_t vA;
    uint64_t vB;
    uint64_t vResult;
    int numElements;
  } addKerParams[totalAddKer];

  // Assign device memory for data - a hard coded hack for now
  auto bufSize = numElems * sizeof(int);
  auto alignedBufSize = ALIGN(bufSize, kCacheLineSize);
  std::vector<uint64_t> dataLoadAddr(queueCount, 0);
  std::vector<uint64_t> devAddrKernelArgs(queueCount, 0);
  std::vector<uint64_t> devAddrkernelException(queueCount, 0);
  std::vector<uint64_t> hostVirtAddrA(queueCount, 0);
  std::vector<uint64_t> hostVirtAddrB(queueCount, 0);
  int addKerCount = 0;
  int kerCount = 0;
  int readCount = 0;
  std::vector<std::vector<int>> outputVec;

  for (uint16_t queueId = 0; queueId < queueCount; queueId++) {
    loadElfToDevice(deviceIdx, addKerReader[queueId], addKerElfPath, stream, addKernelEntryAddr[queueId]);
    loadElfToDevice(deviceIdx, excepKerReader[queueId], excepKerElfPath, stream, excepKerEntryAddr[queueId]);
    loadElfToDevice(deviceIdx, hangKerReader[queueId], hangKerElfPath, stream, hangKerEntryAddr[queueId]);

    // Allocate device space for vector A, B and resultant vectors (totalAddKer)
    dataLoadAddr[queueId] = getDmaWriteAddr(deviceIdx, 2 * alignedBufSize);
    auto devAddrVecA = dataLoadAddr[queueId];
    auto devAddrVecB = devAddrVecA + alignedBufSize;
    // Copy kernel input data to device
    hostVirtAddrA[queueId] = reinterpret_cast<uint64_t>(vA[queueId].data());
    auto hostPhysAddrA = hostVirtAddrA[queueId]; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(
      device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devAddrVecA, hostVirtAddrA[queueId], hostPhysAddrA,
      vA[queueId].size() * sizeof(vA[queueId][0]), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    hostVirtAddrB[queueId] = reinterpret_cast<uint64_t>(vB[queueId].data());
    auto hostPhysAddrB = hostVirtAddrB[queueId]; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(
      device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devAddrVecB, hostVirtAddrB[queueId], hostPhysAddrB,
      vB[queueId].size() * sizeof(vB[queueId][0]), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

    device_ops_api::tag_id_t kernelLaunchTagId;

    for (int i = 0; i < totalKer / queueCount; i++) {
      switch (kerTypes[kerCount]) {
      case addKerType: /* Add Kernel */
        // Copy kernel args to device
        addKerParams[addKerCount].vA = devAddrVecA;
        addKerParams[addKerCount].vB = devAddrVecB;
        addKerParams[addKerCount].vResult = getDmaWriteAddr(deviceIdx, alignedBufSize);
        addKerParams[addKerCount].numElements = numElems;
        devAddrKernelArgs[queueId] = getDmaWriteAddr(deviceIdx, ALIGN(sizeof(Params), kCacheLineSize));
        // allocate 1MB space for kernel error/exception buffer
        devAddrkernelException[queueId] = getDmaWriteAddr(deviceIdx, 0x100000);

        // Launch Kernel Command for add kernel
        stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
          device_ops_api::CMD_FLAGS_BARRIER_ENABLE, addKernelEntryAddr[queueId], devAddrKernelArgs[queueId],
          devAddrkernelException[queueId], shire_mask, 0, reinterpret_cast<void*>(&addKerParams[addKerCount]),
          sizeof(Params), device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));
        addKerCount++;
        kerCount++;
        totalAddKerPerSq[queueId]++;
        break;

      case excKerType:
        // allocate 1MB space for kernel error/exception buffer
        devAddrkernelException[queueId] = getDmaWriteAddr(deviceIdx, 0x100000);
        // Launch Kernel Command for exception kernel
        stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
          device_ops_api::CMD_FLAGS_BARRIER_ENABLE, excepKerEntryAddr[queueId], 0 /* No kernel args */,
          devAddrkernelException[queueId], shire_mask, 0, reinterpret_cast<void*>(&addKerParams[addKerCount]),
          sizeof(Params), device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_EXCEPTION));
        kerCount++;
        break;

      case hangKerType: {
        // allocate 1MB space for kernel error/exception buffer
        devAddrkernelException[queueId] = getDmaWriteAddr(deviceIdx, 0x100000);
        // Kernel launch
        auto kernelCmd = IDevOpsApiCmd::createKernelLaunchCmd(
          device_ops_api::CMD_FLAGS_BARRIER_ENABLE, hangKerEntryAddr[queueId], 0 /* No kernel args */,
          devAddrkernelException[queueId], shire_mask, 0, NULL, 0,
          device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED);
        kernelLaunchTagId = kernelCmd->getCmdTagId();
        stream.push_back(std::move(kernelCmd));
        // Kernel Abort
        stream.push_back(
          IDevOpsApiCmd::createKernelAbortCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kernelLaunchTagId,
                                              device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS));
        kerCount++;
        break;
      }
      default:
        kerCount++;
        break;
      }
    }

    // Read back add Kernel Results from device
    for (int i = 0; i < totalAddKerPerSq[queueId]; i++) {
      std::vector<int> resultFromDevice(numElems, 0);
      auto hostVirtAddrRes = reinterpret_cast<uint64_t>(resultFromDevice.data());
      auto hostPhysAddrRes = hostVirtAddrRes; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(
        i == 0, /* Barrier only for first read to make sure that all kernels execution done */
        addKerParams[readCount].vResult, hostVirtAddrRes, hostPhysAddrRes,
        resultFromDevice.size() * sizeof(resultFromDevice[0]), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      outputVec.push_back(std::move(resultFromDevice));
      readCount++;
    }

    // Move stream of commands to streams_
    streams_.emplace(key(deviceIdx, queueId), std::move(stream));
  }
  executeAsync();

  // Verify Vector's Data
  readCount = 0;
  for (uint16_t queueId = 0; queueId < queueCount; queueId++) {
    for (int i = 0; i < totalAddKerPerSq[queueId]; i++) {
      ASSERT_EQ(outputVec[readCount], vSum[queueId])
        << "outputVec[" << readCount << "] of SQ " << queueId << " result validation failed!";
      readCount++;
    }
  }

  TEST_VLOG(1) << "====> BACK TO BACK " << totalKer << " DIFFERENT KERNEL LAUNCH VERIFIED" << std::endl;
}

void TestDevOpsApiKernelCmds::backToBackEmptyKernelLaunch_3_3(uint64_t shire_mask, bool flushL3) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  int deviceIdx = 0;
  auto queueCount = getSqCount(deviceIdx);

  const int totalKer = 100;
  std::vector<ELFIO::elfio> reader(queueCount);
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("empty.elf")).string();
  std::vector<uint64_t> kernelEntryAddr(queueCount, 0);
  uint64_t devAddrResult[totalKer];
  std::vector<std::vector<uint8_t>> outputVec;
  uint8_t dummyKernelArgs[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF};
  std::vector<uint64_t> devAddrKernelArgs(queueCount, 0);
  std::vector<uint64_t> devAddrkernelException(queueCount, 0);
  int readCount = 0;
  int writeCount = 0;

  for (uint16_t queueId = 0; queueId < queueCount; queueId++) {
    // Load kernel ELF
    loadElfToDevice(deviceIdx, reader[queueId], elfPath, stream, kernelEntryAddr[queueId]);
    for (int i = 0; i < totalKer / queueCount; i++) {
      // allocate 4KB space for dummy kernel result
      devAddrResult[writeCount] = getDmaWriteAddr(deviceIdx, ALIGN(0x1000, kCacheLineSize));
      // allocate space for kernel args
      devAddrKernelArgs[queueId] = getDmaWriteAddr(deviceIdx, ALIGN(sizeof(dummyKernelArgs), kCacheLineSize));
      // allocate 1MB space for kernel error/exception buffer
      devAddrkernelException[queueId] = getDmaWriteAddr(deviceIdx, 0x100000);

      // Launch Kernel Command
      stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
        (device_ops_api::CMD_FLAGS_BARRIER_ENABLE | (flushL3 ? device_ops_api::CMD_FLAGS_KERNEL_LAUNCH_FLUSH_L3 : 0)),
        kernelEntryAddr[queueId], devAddrKernelArgs[queueId], devAddrkernelException[queueId], shire_mask, 0,
        reinterpret_cast<void*>(dummyKernelArgs), sizeof(dummyKernelArgs),
        device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));
      writeCount++;
    }

    // Read back dummy Kernel Results from device
    for (int i = 0; i < totalKer / queueCount; i++) {
      std::vector<uint8_t> resultFromDevice(0x1000, 0);
      auto hostVirtAddrRes = reinterpret_cast<uint64_t>(resultFromDevice.data());
      auto hostPhysAddrRes = hostVirtAddrRes; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataReadCmd(
        i == 0, /* Barrier only for first read to make sure that all kernels execution done */
        devAddrResult[readCount], hostVirtAddrRes, hostPhysAddrRes, resultFromDevice.size(),
        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      outputVec.push_back(std::move(resultFromDevice));
      readCount++;
    }

    // Move stream of commands to streams_
    streams_.emplace(key(deviceIdx, queueId), std::move(stream));
  }

  executeAsync();

  TEST_VLOG(1) << "====> BACK TO BACK " << totalKer << " KERNEL LAUNCH (EMPTY KERNEL) DONE <====\n" << std::endl;
}
