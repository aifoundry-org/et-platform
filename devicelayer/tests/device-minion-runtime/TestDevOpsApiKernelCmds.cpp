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
#include <experimental/filesystem>

using namespace ELFIO;
namespace fs = std::experimental::filesystem;

/**********************************************************
 *                                                         *
 *               Kernel Functional Tests                   *
 *                                                         *
 **********************************************************/
void TestDevOpsApiKernelCmds::launchAddVectorKernel_PositiveTesting_4_1(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0x91);

  // Load ELF
  ELFIO::elfio reader;
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("add_vector.elf")).string();
  uint64_t kernelEntryAddr;
  loadElfToDevice(reader, elfPath, stream, kernelEntryAddr);

  // Create vector data
  std::vector<int> vA, vB, vResult;
  int numElems = 10496;
  for (int i = 0; i < numElems; ++i) {
    vA.emplace_back(rand());
    vB.emplace_back(rand());
    vResult.emplace_back(vA.back() + vB.back());
  }

  // Assign device memory for data - a hard coded hack for now
  auto bufSize = numElems * sizeof(int);
  auto alignedBufSize = ALIGN(bufSize, kCacheLineSize);
  auto dataLoadAddr = getDmaWriteAddr(3 * alignedBufSize);
  auto devAddrVecA = dataLoadAddr;
  auto devAddrVecB = devAddrVecA + alignedBufSize;
  auto devAddrVecResult = devAddrVecB + alignedBufSize;

  // create device params structure
  struct Params {
    uint64_t vA;
    uint64_t vB;
    uint64_t vResult;
    int numElements;
  } parameters{devAddrVecA, devAddrVecB, devAddrVecResult, numElems};
  auto sizeOfParams = sizeof(parameters);

  // Copy kernel input data to device
  auto hostVirtAddr = reinterpret_cast<uint64_t>(vA.data());
  auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devAddrVecA, hostVirtAddr, hostPhysAddr,
                                                     vA.size() * sizeof(vA[0]),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
  hostVirtAddr = reinterpret_cast<uint64_t>(vB.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devAddrVecB, hostVirtAddr, hostPhysAddr,
                                                     vB.size() * sizeof(vB[0]),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Copy kernel args to device
  auto devAddrKernelArgs = getDmaWriteAddr(0x400);
  hostVirtAddr = reinterpret_cast<uint64_t>(&parameters);
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devAddrKernelArgs, hostVirtAddr,
                                                     hostPhysAddr, sizeof(parameters),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Launch Kernel Command
  stream.push_back(
    IDevOpsApiCmd::createKernelLaunchCmd(getNextTagId(), true, kernelEntryAddr, devAddrKernelArgs, shire_mask,
                                         device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));

  // Read back Kernel Results from device
  std::vector<int> resultFromDevice(numElems, 0);
  hostVirtAddr = reinterpret_cast<uint64_t>(resultFromDevice.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devAddrVecResult, hostVirtAddr, hostPhysAddr,
                                                    resultFromDevice.size() * sizeof(resultFromDevice[0]),
                                                    device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  // Verify Vector's Data
  ASSERT_EQ(resultFromDevice, vResult);
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
  };

  layer_parameters_t launchArgs[2];
  uint64_t devAddrKernelArgs = getDmaWriteAddr(sizeof(launchArgs));
  launchArgs[0].data_ptr = devAddrBufLayer0;
  launchArgs[0].length = bufSizeLayer0;
  launchArgs[1].data_ptr = devAddrBufLayer1;
  launchArgs[1].length = bufSizeLayer1;

  // Copy kernel launch args
  auto hostVirtAddr = reinterpret_cast<uint64_t>(launchArgs);
  auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devAddrKernelArgs, hostVirtAddr,
                                                     hostPhysAddr, sizeof(launchArgs),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
  // Kernel launch Command
  stream.push_back(
    IDevOpsApiCmd::createKernelLaunchCmd(getNextTagId(), true, kernelEntryAddr, devAddrKernelArgs, shire_mask,
                                         device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));

  // Read back data written by layer 0
  std::vector<uint64_t> resultDataLayer0(numElemsLayer0, 0xEEEEEEEEEEEEEEEEULL);
  hostVirtAddr = reinterpret_cast<uint64_t>(resultDataLayer0.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devAddrBufLayer0, hostVirtAddr, hostPhysAddr,
                                                    resultDataLayer0.size() * sizeof(resultDataLayer0[0]),
                                                    device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Read back data written by layer 1
  std::vector<uint64_t> resultDataLayer1(numElemsLayer1, 0xEEEEEEEEEEEEEEEEULL);
  hostVirtAddr = reinterpret_cast<uint64_t>(resultDataLayer1.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), true, devAddrBufLayer1, hostVirtAddr, hostPhysAddr,
                                                    resultDataLayer1.size() * sizeof(resultDataLayer1[0]),
                                                    device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  // Verify data
  const std::vector<uint64_t> refdata0(numElemsLayer0, 0xBEEFBEEFBEEFBEEFULL);
  ASSERT_EQ(resultDataLayer0, refdata0);

  const std::vector<uint64_t> refdata1(numElemsLayer1, 0xBEEFBEEFBEEFBEEFULL);
  ASSERT_EQ(resultDataLayer1, refdata1);

  TEST_VLOG(1) << "====> UBERKERNEL KERNEL RESPONSE DATA VERIFIED <====" << std::endl;
}

void TestDevOpsApiKernelCmds::launchExceptionKernel_NegativeTesting_4_6(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0xb1);

  // Load ELF
  ELFIO::elfio reader;
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("exception.elf")).string();
  uint64_t kernelEntryAddr;
  loadElfToDevice(reader, elfPath, stream, kernelEntryAddr);

  // Kernel launch
  stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(getNextTagId(), false, kernelEntryAddr, 0 /* No kernel args */,
                                                        shire_mask,
                                                        device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_EXCEPTION));

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  TEST_VLOG(1) << "====> EXCEPTION KERNEL DONE <====" << std::endl;
}

void TestDevOpsApiKernelCmds::abortHangKernel_PositiveTesting_4_10(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0xb1);

  // Load ELF
  ELFIO::elfio reader;
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("hang.elf")).string();
  uint64_t kernelEntryAddr;
  loadElfToDevice(reader, elfPath, stream, kernelEntryAddr);

  // Kernel launch
  auto kernelLaunchTagId = getNextTagId();
  stream.push_back(
    IDevOpsApiCmd::createKernelLaunchCmd(kernelLaunchTagId, false, kernelEntryAddr, 0 /* No kernel args */, shire_mask,
                                         device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED));

  // Should we add some delay before sending the abort?

  // Kernel Abort
  stream.push_back(IDevOpsApiCmd::createKernelAbortCmd(getNextTagId(), false, kernelLaunchTagId,
                                                       device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS));

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

  TEST_VLOG(1) << "====> HANG KERNEL DONE <====" << std::endl;
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
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devAddrVecA, hostVirtAddrVecA,
                                                       hostPhysAddrVecA, vA.size() * sizeof(vA[0]),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devAddrVecB, hostVirtAddrVecB,
                                                       hostPhysAddrVecB, vB.size() * sizeof(vB[0]),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

    // Copy kernel args to device
    devAddrVecResult[i] = devAddrVecB + alignedBufSize;
    auto devAddrKernelArgs = getDmaWriteAddr(ALIGN(sizeof(Params), kCacheLineSize));
    kerParams[i] = {devAddrVecA, devAddrVecB, devAddrVecResult[i], numElems};
    auto hostVirtAddrKer = reinterpret_cast<uint64_t>(&kerParams[i]);
    auto hostPhysAddrKer = hostVirtAddrKer; // Should be handled in SysEmu, userspace should not fill this value
    stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devAddrKernelArgs, hostVirtAddrKer,
                                                       hostPhysAddrKer, sizeof(Params),
                                                       device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

    // Launch Kernel Command
    stream.push_back(
      IDevOpsApiCmd::createKernelLaunchCmd(getNextTagId(), true, kernelEntryAddr, devAddrKernelArgs, shire_mask,
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
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devAddrVecA, hostVirtAddr, hostPhysAddr,
                                                     vA.size() * sizeof(vA[0]),
                                                     device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
  hostVirtAddr = reinterpret_cast<uint64_t>(vB.data());
  hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devAddrVecB, hostVirtAddr, hostPhysAddr,
                                                     vB.size() * sizeof(vB[0]),
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
      hostVirtAddr = reinterpret_cast<uint64_t>(&addKerParams[addKerCount]);
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      stream.push_back(IDevOpsApiCmd::createDataWriteCmd(getNextTagId(), false, devAddrKernelArgs, hostVirtAddr,
                                                         hostPhysAddr, sizeof(Params),
                                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      // Launch Kernel Command for add kernel
      stream.push_back(
        IDevOpsApiCmd::createKernelLaunchCmd(getNextTagId(), true, addKernelEntryAddr, devAddrKernelArgs, shire_mask,
                                             device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));
      devAddrVecResult += alignedBufSize;
      addKerCount++;
      break;

    case excKerType:
      // Launch Kernel Command for exception kernel
      stream.push_back(
        IDevOpsApiCmd::createKernelLaunchCmd(getNextTagId(), true, excepKerEntryAddr, 0 /* No kernel args */,
                                             shire_mask, device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_EXCEPTION));
      break;

    case hangKerType:
      // Kernel launch
      kernelLaunchTagId = getNextTagId();
      stream.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
        kernelLaunchTagId, true, hangKerEntryAddr, 0 /* No kernel args */, shire_mask,
        device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED));
      // Kernel Abort
      stream.push_back(IDevOpsApiCmd::createKernelAbortCmd(getNextTagId(), false, kernelLaunchTagId,
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

void TestDevOpsApiKernelCmds::kernelAbortCmd_InvalidTagIdNegativeTesting_6_2() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0xb1);
  
  stream.push_back(IDevOpsApiCmd::createKernelAbortCmd(getNextTagId(), false, 0xbeef /* invalid tagId */,
                                                       device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_INVALID_TAG_ID));

  // Move stream of commands to streams_[queueId]
  uint16_t queueId = 0;
  streams_.emplace(queueId, std::move(stream));

  executeAsync();

}
