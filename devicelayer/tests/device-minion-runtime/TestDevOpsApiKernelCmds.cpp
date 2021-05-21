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
  auto queueCount = devLayer_->getSubmissionQueuesCount(kIDevice);
  initTagId(0x91);

  // Load ELF
  std::array<ELFIO::elfio, maxQueueCount> reader;
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("add_vector.elf")).string();
  uint64_t kernelEntryAddr[queueCount];

  // Create vector data
  std::array<std::vector<int>, maxQueueCount> vA;
  std::array<std::vector<int>, maxQueueCount> vB;
  std::array<std::vector<int>, maxQueueCount> vResult;
  int numElems = 10496;

  std::vector<std::vector<int>> resultFromDevice;
  std::array<std::vector<int>, maxQueueCount> readBuf;

  // Assign device memory for data
  auto bufSize = numElems * sizeof(int);
  auto alignedBufSize = ALIGN(bufSize, kCacheLineSize);
  std::array<uint64_t, maxQueueCount> dataLoadAddr;
  std::array<uint64_t, maxQueueCount> devAddrVecA;
  std::array<uint64_t, maxQueueCount> devAddrVecB;
  std::array<uint64_t, maxQueueCount> devAddrVecResult;

  // create device params structure
  struct Params {
    uint64_t vA;
    uint64_t vB;
    uint64_t vResult;
    int numElements;
  };
  std::array<Params, maxQueueCount> parameters;

  std::minstd_rand simple_rand;
  simple_rand.seed(time(NULL));

  for (int queueId = 0; queueId < queueCount; queueId++) {
    stream.clear();

    for (int i = 0; i < numElems; ++i) {
      vA.at(queueId).emplace_back(simple_rand());
      vB.at(queueId).emplace_back(simple_rand());
      vResult.at(queueId).emplace_back(vA.at(queueId).back() + vB.at(queueId).back());
    }

    dataLoadAddr.at(queueId) = getDmaWriteAddr(3 * alignedBufSize);
    devAddrVecA.at(queueId) = dataLoadAddr.at(queueId);
    devAddrVecB.at(queueId) = devAddrVecA.at(queueId) + alignedBufSize;
    devAddrVecResult.at(queueId) = devAddrVecB.at(queueId) + alignedBufSize;

    parameters.at(queueId) = {devAddrVecA.at(queueId), devAddrVecB.at(queueId), devAddrVecResult.at(queueId), numElems};
    auto sizeOfParams = sizeof(parameters.at(queueId));

    // Load ELF
    loadElfToDevice(reader.at(queueId), elfPath, stream, kernelEntryAddr[queueId]);
    // Copy kernel input data to device
    auto hostVirtAddr = reinterpret_cast<uint64_t>(vA.at(queueId).data());
    auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devAddrVecA.at(queueId), hostVirtAddr,
                                                       hostPhysAddr, vA.at(queueId).size() * sizeof(vA.at(queueId)[0]),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    hostVirtAddr = reinterpret_cast<uint64_t>(vB.at(queueId).data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devAddrVecB.at(queueId), hostVirtAddr,
                                                       hostPhysAddr, vB.at(queueId).size() * sizeof(vB.at(queueId)[0]),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

    // allocate space for kernel args
    auto devAddrKernelArgs = getDmaWriteAddr(0x400);

    // allocate 1MB space for kernel error/exception buffer
    auto devAddrkernelException = getDmaWriteAddr(0x100000);

    // Launch Kernel Command
    stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
      getNextTagId(), true, kernelEntryAddr[queueId], devAddrKernelArgs, devAddrkernelException, shire_mask, 0,
      reinterpret_cast<void*>(&parameters.at(queueId)), sizeOfParams,
      device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));

    // Read back Kernel Results from device
    readBuf.at(queueId).resize(numElems, 0);
    hostVirtAddr = reinterpret_cast<uint64_t>(readBuf.at(queueId).data());
    hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(
      getNextTagId(), true, devAddrVecResult.at(queueId), hostVirtAddr, hostPhysAddr,
      readBuf.at(queueId).size() * sizeof(readBuf.at(queueId)[0]), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    resultFromDevice.push_back(std::move(readBuf.at(queueId)));
    // Move stream of commands to streams_[queueId]
    streams_.emplace(queueId, std::move(stream));
  }

  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // Verify Vector's Data
  for (int queueId = 0; queueId < queueCount; queueId++) {
    ASSERT_EQ(resultFromDevice[queueId], vResult.at(queueId));
  }

  TEST_VLOG(1) << "====> ADD TWO VECTORS KERNEL RESPONSE DATA VERIFIED <====" << std::endl;
}

void TestDevOpsApiKernelCmds::launchUberKernel_PositiveTesting_4_4(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0xa1);

  // Load ELF
  ELFIO::elfio reader;
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("uberkernel.elf")).string();
  uint64_t kernelEntryAddr;
  loadElfToDevice(reader, elfPath, stream, kernelEntryAddr);

  // Allocate device buffer for layer 0
  const uint64_t numElemsLayer0 = 200;
  const uint64_t bufSizeLayer0 = sizeof(uint64_t) * numElemsLayer0;
  uint64_t devAddrBufLayer0 = getDmaWriteAddr(bufSizeLayer0);
  TEST_VLOG(1) << "devAddrBufLayer0: 0x" << std::hex << devAddrBufLayer0 << std::endl;

  // Allocate device buffer for layer 1
  const uint64_t numElemsLayer1 = 400;
  const uint64_t bufSizeLayer1 = sizeof(uint64_t) * numElemsLayer1;
  uint64_t devAddrBufLayer1 = getDmaWriteAddr(bufSizeLayer1);
  TEST_VLOG(1) << "devAddrBufLayer1: 0x" << std::hex << devAddrBufLayer1 << std::endl;

  // Setup and allocate kernel launch args
  struct layer_parameters_t {
    uint64_t data_ptr;
    uint64_t length;
    uint32_t shire_count;
  };

  layer_parameters_t launchArgs[2];
  auto devAddrKernelArgs = getDmaWriteAddr(ALIGN(sizeof(launchArgs), kCacheLineSize));
  launchArgs[0].data_ptr = devAddrBufLayer0;
  launchArgs[0].length = bufSizeLayer0;
  launchArgs[0].shire_count = std::bitset<64>(shire_mask).count();
  launchArgs[1].data_ptr = devAddrBufLayer1;
  launchArgs[1].length = bufSizeLayer1;
  launchArgs[1].shire_count = std::bitset<64>(shire_mask).count();

  // allocate 1MB space for kernel error/exception buffer
  auto devAddrkernelException = getDmaWriteAddr(0x100000);

  // Kernel launch Command
  stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
    getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_ENABLE, kernelEntryAddr, devAddrKernelArgs,
    devAddrkernelException, shire_mask, 0, reinterpret_cast<void*>(launchArgs), sizeof(launchArgs),
    device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));

  // Read back data written by layer 0
  std::vector<uint64_t> resultDataLayer0(numElemsLayer0, 0xEEEEEEEEEEEEEEEEULL);
  auto hostVirtAddr = reinterpret_cast<uint64_t>(resultDataLayer0.data());
  auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(
    getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrBufLayer0, hostVirtAddr, hostPhysAddr,
    resultDataLayer0.size() * sizeof(resultDataLayer0[0]), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Read back data written by layer 1
  std::vector<uint64_t> resultDataLayer1(numElemsLayer1, 0xEEEEEEEEEEEEEEEEULL);
  hostVirtAddr = reinterpret_cast<uint64_t>(resultDataLayer1.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(
    getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrBufLayer1, hostVirtAddr, hostPhysAddr,
    resultDataLayer1.size() * sizeof(resultDataLayer1[0]), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // Verify data
  const std::vector<uint64_t> refdata0(numElemsLayer0, 0xBEEFBEEFBEEFBEEFULL);
  ASSERT_EQ(resultDataLayer0, refdata0);

  const std::vector<uint64_t> refdata1(numElemsLayer1, 0xBEEFBEEFBEEFBEEFULL);
  ASSERT_EQ(resultDataLayer1, refdata1);

  TEST_VLOG(1) << "====> UBERKERNEL KERNEL RESPONSE DATA VERIFIED <====" << std::endl;
}

void TestDevOpsApiKernelCmds::launchEmptyKernel_PositiveTesting_4_5(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0xb1);

  // Load ELF
  ELFIO::elfio reader;
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("empty.elf")).string();
  uint64_t kernelEntryAddr;
  loadElfToDevice(reader, elfPath, stream, kernelEntryAddr);

  uint8_t dummyKernelArgs[] = {0xDE, 0xAD, 0xBE, 0xEF};
  // allocate space for kernel args
  auto devAddrKernelArgs = getDmaWriteAddr(ALIGN(sizeof(dummyKernelArgs), kCacheLineSize));
  // allocate 4KB space for dummy kernel result
  auto devAddrKernelResult = getDmaWriteAddr(ALIGN(sizeof(0x1000), kCacheLineSize));
  // allocate 1MB space for kernel error/exception buffer
  auto devAddrkernelException = getDmaWriteAddr(0x100000);

  // kernel launch
  stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
    getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kernelEntryAddr, devAddrKernelArgs,
    devAddrkernelException, shire_mask, 0, reinterpret_cast<void*>(dummyKernelArgs), sizeof(dummyKernelArgs),
    device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));
  // Do a dummy read back Kernel Results from device
  std::vector<uint8_t> resultFromDevice(0x1000, 0);
  auto hostVirtAddr = reinterpret_cast<uint64_t>(resultFromDevice.data());
  auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(
    getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrKernelResult, hostVirtAddr, hostPhysAddr,
    resultFromDevice.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  TEST_VLOG(1) << "====> EMPTY KERNEL DONE <====" << std::endl;
}

void TestDevOpsApiKernelCmds::launchExceptionKernel_NegativeTesting_4_6(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0xc1);

  // Load ELF
  ELFIO::elfio reader;
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("exception.elf")).string();
  uint64_t kernelEntryAddr;
  loadElfToDevice(reader, elfPath, stream, kernelEntryAddr);

  // allocate 1MB space for kernel error/exception buffer
  auto devAddrkernelException = getDmaWriteAddr(0x100000);

  // Kernel launch
  stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
    getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kernelEntryAddr, 0 /* No kernel args */,
    devAddrkernelException, shire_mask, 0, NULL, 0, device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_EXCEPTION));

  // pull the exception buffer from device
  std::vector<uint8_t> resultFromDevice(0x100000, 0);
  auto hostVirtAddr = reinterpret_cast<uint64_t>(resultFromDevice.data());
  auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(
    getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrkernelException, hostVirtAddr, hostPhysAddr,
    resultFromDevice.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  // print the exception buffer scause for the first shire
  printErrorContext(reinterpret_cast<void*>(resultFromDevice.data()), 0x1);

  TEST_VLOG(1) << "====> EXCEPTION KERNEL DONE <====" << std::endl;
}

void TestDevOpsApiKernelCmds::abortHangKernel_PositiveTesting_4_10(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0xd1);

  // Load ELF
  ELFIO::elfio reader;
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("hang.elf")).string();
  uint64_t kernelEntryAddr;
  loadElfToDevice(reader, elfPath, stream, kernelEntryAddr);

  // allocate 1MB space for kernel error/exception buffer
  auto devAddrkernelException = getDmaWriteAddr(0x100000);

  // Kernel launch
  auto kernelLaunchTagId = getNextTagId();
  stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
    kernelLaunchTagId, device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kernelEntryAddr, 0 /* No kernel args */,
    devAddrkernelException, shire_mask, 0, NULL, 0, device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED));

  // Should we add some delay before sending the abort?

  // Kernel Abort
  stream.push_back(IDevOpsApiCmd::createKernelAbortCmd(getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_DISABLE,
                                                       kernelLaunchTagId,
                                                       device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS));

  // pull the exception buffer from device
  std::vector<uint8_t> resultFromDevice(0x100000, 0);
  auto hostVirtAddr = reinterpret_cast<uint64_t>(resultFromDevice.data());
  auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(
    getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrkernelException, hostVirtAddr, hostPhysAddr,
    resultFromDevice.size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  // print the exception buffer scause for the first two shires
  printErrorContext(reinterpret_cast<void*>(resultFromDevice.data()), 0x3);

  TEST_VLOG(1) << "====> HANG KERNEL DONE <====" << std::endl;
}

void TestDevOpsApiKernelCmds::kernelAbortCmd_InvalidTagIdNegativeTesting_6_2() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0xe1);

  stream.push_back(IDevOpsApiCmd::createKernelAbortCmd(
    getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_DISABLE, 0xbeef /* invalid tagId */,
    device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_INVALID_TAG_ID));

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();
}

/**********************************************************
 *                                                         *
 *                  Kernel Stress Tests                    *
 *                                                         *
 **********************************************************/
void TestDevOpsApiKernelCmds::backToBackSameKernelLaunchCmds_3_1(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0xc1);

  const int totalKer = 100;
  // create device params structure
  struct Params {
    uint64_t vA;
    uint64_t vB;
    uint64_t vResult;
    int numElements;
  } kerParams[totalKer];
  ELFIO::elfio reader;
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("add_vector.elf")).string();
  uint64_t kernelEntryAddr;
  int numElems = 4 * 1024;
  auto bufSize = numElems * sizeof(int);
  auto alignedBufSize = ALIGN(bufSize, kCacheLineSize);

  // Create vector data
  std::vector<int> vA, vB, vSum;
  for (int i = 0; i < numElems; ++i) {
    vA.emplace_back(rand() % 1000);
    vB.emplace_back(rand() % 1000);
    vSum.emplace_back(vA.back() + vB.back());
  }
  auto hostVirtAddrVecA = reinterpret_cast<uint64_t>(vA.data());
  auto hostPhysAddrVecA = hostVirtAddrVecA; // Should be handled in SysEmu, userspace should not fill this value
  auto hostVirtAddrVecB = reinterpret_cast<uint64_t>(vB.data());
  auto hostPhysAddrVecB = hostVirtAddrVecB; // Should be handled in SysEmu, userspace should not fill this value
  uint64_t devAddrVecResult[totalKer];
  std::vector<std::vector<int>> outputVec;

  // Load kernel ELF
  loadElfToDevice(reader, elfPath, stream, kernelEntryAddr);
  for (int i = 0; i < totalKer; i++) {
    // Copy kernel input data to device
    auto dataLoadAddr = getDmaWriteAddr(3 * alignedBufSize);
    auto devAddrVecA = dataLoadAddr;
    auto devAddrVecB = devAddrVecA + alignedBufSize;
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(
      getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devAddrVecA, hostVirtAddrVecA, hostPhysAddrVecA,
      vA.size() * sizeof(vA[0]), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(
      getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devAddrVecB, hostVirtAddrVecB, hostPhysAddrVecB,
      vB.size() * sizeof(vB[0]), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

    // allocate space for kernel args
    devAddrVecResult[i] = devAddrVecB + alignedBufSize;
    auto devAddrKernelArgs = getDmaWriteAddr(ALIGN(sizeof(Params), kCacheLineSize));
    kerParams[i] = {devAddrVecA, devAddrVecB, devAddrVecResult[i], numElems};

    // allocate 1MB space for kernel error/exception buffer
    auto devAddrkernelException = getDmaWriteAddr(0x100000);

    // Launch Kernel Command
    stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
      getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_ENABLE, kernelEntryAddr, devAddrKernelArgs,
      devAddrkernelException, shire_mask, 0, reinterpret_cast<void*>(&kerParams[i]), sizeof(Params),
      device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));
  }

  // Read back Kernel Results from device
  for (int i = 0; i < totalKer; i++) {
    std::vector<int> resultFromDevice(numElems, 0);
    auto hostVirtAddrRes = reinterpret_cast<uint64_t>(resultFromDevice.data());
    auto hostPhysAddrRes = hostVirtAddrRes; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(
      getNextTagId(), i == 0, /* Barrier only for first read to make sure that all kernels execution done */
      devAddrVecResult[i], hostVirtAddrRes, hostPhysAddrRes, resultFromDevice.size() * sizeof(resultFromDevice[0]),
      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    outputVec.push_back(std::move(resultFromDevice));
  }

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // Verify Vector's Data
  for (int i = 0; i < totalKer; i++) {
    ASSERT_EQ(outputVec[i], vSum) << "outputVec[" << i << "] result validation failed!";
  }

  TEST_VLOG(1) << "====> BACK TO BACK " << totalKer << " KERNEL LAUNCH (ADD VECTORS KERNEL) DATA VERIFIED <====\n"
               << std::endl;
}

void TestDevOpsApiKernelCmds::backToBackDifferentKernelLaunchCmds_3_2(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0xd1);

  const int totalKer = 100;

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
  ELFIO::elfio addKerReader;
  auto addKerElfPath = (fs::path(FLAGS_kernels_dir) / fs::path("add_vector.elf")).string();
  uint64_t addKernelEntryAddr;
  loadElfToDevice(addKerReader, addKerElfPath, stream, addKernelEntryAddr);
  // Load exception ELF kerTypes = 1
  ELFIO::elfio excepKerReader;
  auto excepKerElfPath = (fs::path(FLAGS_kernels_dir) / fs::path("exception.elf")).string();
  uint64_t excepKerEntryAddr;
  loadElfToDevice(excepKerReader, excepKerElfPath, stream, excepKerEntryAddr);
  // Load hang ELF kerTypes = 2
  ELFIO::elfio hangKerReader;
  auto hangKerElfPath = (fs::path(FLAGS_kernels_dir) / fs::path("hang.elf")).string();
  uint64_t hangKerEntryAddr;
  loadElfToDevice(hangKerReader, hangKerElfPath, stream, hangKerEntryAddr);

  /*******************************************************
   *                  Setup for Add Kernel               *
   * *****************************************************/
  // Create vector data
  std::vector<int> vA, vB, vSum;
  int numElems = 1024;
  for (int i = 0; i < numElems; ++i) {
    vA.emplace_back(rand() % 1000);
    vB.emplace_back(rand() % 1000);
    vSum.emplace_back(vA.back() + vB.back());
  }
  // Assign device memory for data - a hard coded hack for now
  auto bufSize = numElems * sizeof(int);
  auto alignedBufSize = ALIGN(bufSize, kCacheLineSize);
  // Allocate device space for vector A, B and resultant vectors (totalAddKer)
  auto dataLoadAddr = getDmaWriteAddr((2 + totalAddKer) * alignedBufSize);
  auto devAddrVecA = dataLoadAddr;
  auto devAddrVecB = devAddrVecA + alignedBufSize;
  auto devAddrVecResult = devAddrVecB + alignedBufSize;
  // Copy kernel input data to device
  auto hostVirtAddr = reinterpret_cast<uint64_t>(vA.data());
  auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_DISABLE,
                                                     devAddrVecA, hostVirtAddr, hostPhysAddr, vA.size() * sizeof(vA[0]),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
  hostVirtAddr = reinterpret_cast<uint64_t>(vB.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_DISABLE,
                                                     devAddrVecB, hostVirtAddr, hostPhysAddr, vB.size() * sizeof(vB[0]),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
  // create device params structure
  struct Params {
    uint64_t vA;
    uint64_t vB;
    uint64_t vResult;
    int numElements;
  } addKerParams[totalAddKer];

  int addKerCount = 0;
  uint64_t devAddrKernelArgs;
  uint64_t devAddrkernelException;
  device_ops_api::tag_id_t kernelLaunchTagId;

  for (int i = 0; i < totalKer; i++) {
    switch (kerTypes[i]) {
    case addKerType: /* Add Kernel */
      // Copy kernel args to device
      addKerParams[addKerCount].vA = devAddrVecA;
      addKerParams[addKerCount].vB = devAddrVecB;
      addKerParams[addKerCount].vResult = devAddrVecResult;
      addKerParams[addKerCount].numElements = numElems;
      devAddrKernelArgs = getDmaWriteAddr(ALIGN(sizeof(Params), kCacheLineSize));
      // allocate 1MB space for kernel error/exception buffer
      devAddrkernelException = getDmaWriteAddr(0x100000);

      // Launch Kernel Command for add kernel
      stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
        getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_ENABLE, addKernelEntryAddr, devAddrKernelArgs,
        devAddrkernelException, shire_mask, 0, reinterpret_cast<void*>(&addKerParams[addKerCount]), sizeof(Params),
        device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));
      devAddrVecResult += alignedBufSize;
      addKerCount++;
      break;

    case excKerType:
      // allocate 1MB space for kernel error/exception buffer
      devAddrkernelException = getDmaWriteAddr(0x100000);
      // Launch Kernel Command for exception kernel
      stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
        getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_ENABLE, excepKerEntryAddr, 0 /* No kernel args */,
        devAddrkernelException, shire_mask, 0, NULL, 0, device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_EXCEPTION));
      break;

    case hangKerType:
      // allocate 1MB space for kernel error/exception buffer
      devAddrkernelException = getDmaWriteAddr(0x100000);
      // Kernel launch
      kernelLaunchTagId = getNextTagId();
      stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
        kernelLaunchTagId, device_ops_api::CMD_FLAGS_BARRIER_ENABLE, hangKerEntryAddr, 0 /* No kernel args */,
        devAddrkernelException, shire_mask, 0, NULL, 0,
        device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED));
      // Kernel Abort
      stream.push_back(IDevOpsApiCmd::createKernelAbortCmd(getNextTagId(), device_ops_api::CMD_FLAGS_BARRIER_DISABLE,
                                                           kernelLaunchTagId,
                                                           device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS));
      break;

    default:
      break;
    }
  }

  // Read back add Kernel Results from device
  std::vector<std::vector<int>> outputVec;
  for (int i = 0; i < totalAddKer; i++) {
    std::vector<int> resultFromDevice(numElems, 0);
    auto hostVirtAddrRes = reinterpret_cast<uint64_t>(resultFromDevice.data());
    auto hostPhysAddrRes = hostVirtAddrRes; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(
      getNextTagId(), i == 0, /* Barrier only for first read to make sure that all kernels execution done */
      addKerParams[i].vResult, hostVirtAddrRes, hostPhysAddrRes, resultFromDevice.size() * sizeof(resultFromDevice[0]),
      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    outputVec.push_back(std::move(resultFromDevice));
  }

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  // Verify Vector's Data
  for (int i = 0; i < totalAddKer; i++) {
    ASSERT_EQ(outputVec[i], vSum) << "outputVec[" << i << "] result validation failed!";
  }

  TEST_VLOG(1) << "====> BACK TO BACK " << totalKer << " DIFFERENT KERNEL LAUNCH VERIFIED" << std::endl;
}

void TestDevOpsApiKernelCmds::backToBackEmptyKernelLaunch_3_3(uint64_t shire_mask, bool flushL3) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0xe1);

  const int totalKer = 100;
  ELFIO::elfio reader;
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("empty.elf")).string();
  uint64_t kernelEntryAddr;
  uint64_t devAddrResult[totalKer];
  std::vector<std::vector<uint8_t>> outputVec;
  uint8_t dummyKernelArgs[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF};

  // Load kernel ELF
  loadElfToDevice(reader, elfPath, stream, kernelEntryAddr);
  for (int i = 0; i < totalKer; i++) {
    // allocate 4KB space for dummy kernel result
    devAddrResult[i] = getDmaWriteAddr(ALIGN(0x1000, kCacheLineSize));
    // allocate space for kernel args
    auto devAddrKernelArgs = getDmaWriteAddr(ALIGN(sizeof(dummyKernelArgs), kCacheLineSize));
    // allocate 1MB space for kernel error/exception buffer
    auto devAddrkernelException = getDmaWriteAddr(0x100000);

    // Launch Kernel Command
    stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
      getNextTagId(),
      (device_ops_api::CMD_FLAGS_BARRIER_ENABLE | (flushL3 ? device_ops_api::CMD_FLAGS_KERNEL_LAUNCH_FLUSH_L3 : 0)),
      kernelEntryAddr, devAddrKernelArgs, devAddrkernelException, shire_mask, 0,
      reinterpret_cast<void*>(dummyKernelArgs), sizeof(dummyKernelArgs),
      device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));
  }

  // Read back dummy Kernel Results from device
  for (int i = 0; i < totalKer; i++) {
    std::vector<uint8_t> resultFromDevice(0x1000, 0);
    auto hostVirtAddrRes = reinterpret_cast<uint64_t>(resultFromDevice.data());
    auto hostPhysAddrRes = hostVirtAddrRes; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataReadCmd(
      getNextTagId(), i == 0, /* Barrier only for first read to make sure that all kernels execution done */
      devAddrResult[i], hostVirtAddrRes, hostPhysAddrRes, resultFromDevice.size(),
      device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    outputVec.push_back(std::move(resultFromDevice));
  }

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  TEST_VLOG(1) << "====> BACK TO BACK " << totalKer << " KERNEL LAUNCH (EMPTY KERNEL) DONE <====\n" << std::endl;
}
