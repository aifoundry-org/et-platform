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
using namespace dev::dl_tests;
namespace fs = std::experimental::filesystem;

const uint64_t kExceptionSize = 1024 * 1024; // 1 MB
const uint64_t kEmptyKerDummyRead = 1024 * 4; // 4KB
const uint64_t kUberKerLayerBuf0 = 200 * sizeof(uint64_t);
const uint64_t kUberKerLayerBuf1 = 400 * sizeof(uint64_t);

void static generateRandomData(int totalNumbers, std::vector<std::vector<int>>& dataAStorage,
                               std::vector<std::vector<int>>& dataBStorage,
                               std::vector<std::vector<int>>& dataSumStorage) {
  std::vector<int> tempDataA;
  std::vector<int> tempDataB;
  std::vector<int> tempDataSum;

  std::minstd_rand simple_rand;
  simple_rand.seed(time(nullptr));

  tempDataA.resize(totalNumbers, 0);
  tempDataB.resize(totalNumbers, 0);
  tempDataSum.resize(totalNumbers, 0);
  for (int i = 0; i < totalNumbers; i++) {
    tempDataA[i] = simple_rand();
    tempDataB[i] = simple_rand();
    tempDataSum[i] = tempDataA[i] + tempDataB[i];
  }
  dataAStorage.push_back(std::move(tempDataA));
  dataBStorage.push_back(std::move(tempDataB));
  dataSumStorage.push_back(std::move(tempDataSum));
}

static std::vector<int>
addKernelRandomData(int numElems, int* vecA, int* vecB) {
  std::vector<int> sumOfAB(numElems);

  std::minstd_rand simple_rand;
  simple_rand.seed(time(nullptr));
  for (int i = 0; i < numElems; ++i) {
      vecA[i] = static_cast<int>(simple_rand());
      vecB[i] = static_cast<int>(simple_rand());
      sumOfAB[i] = vecA[i] + vecB[i];
    }

  return sumOfAB;
}

/**********************************************************
 *                                                         *
 *               Kernel Functional Tests                   *
 *                                                         *
 **********************************************************/
void TestDevOpsApiKernelCmds::launchAddVectorKernel_PositiveTesting_4_1(uint64_t shire_mask, std::string kernelName) {
  std::vector<CmdTag> streamAddKer;

  std::vector<std::vector<ELFIO::elfio>> readersStorageAddKer;
  std::vector<ELFIO::elfio> readersAddKer;
  auto elfPathAddKer = (fs::path(FLAGS_kernels_dir) / fs::path(kernelName)).string();

  // Create vector data
  std::vector<std::vector<int>> vDataAStorageAddKer;
  std::vector<std::vector<int>> vDataBStorageAddKer;
  std::vector<std::vector<int>> vSumStorageAddKer;
  std::vector<std::vector<int>> vResultStorageAddKer;
  std::vector<int> vTempDataResultAddKer;
  int numElemsAddKer = 10496;

  // Assign device memory for data
  auto bufSize = numElemsAddKer * sizeof(int);
  auto alignedBufSize = ALIGN(bufSize, kCacheLineSize);

  int deviceCountAddKer = getDevicesCount();
  for (int deviceIdxAddKer = 0; deviceIdxAddKer < deviceCountAddKer; deviceIdxAddKer++) {
    int queueCountAddKer = getSqCount(deviceIdxAddKer);
    readersAddKer.resize(queueCountAddKer);
    for (int queueIdxAddKer = 0; queueIdxAddKer < queueCountAddKer; queueIdxAddKer++) {
      generateRandomData(numElemsAddKer, vDataAStorageAddKer, vDataBStorageAddKer, vSumStorageAddKer);

      auto vADevAddr = getDmaWriteAddr(deviceIdxAddKer, 3 * alignedBufSize);
      auto vBDevAddr = vADevAddr + alignedBufSize;
      auto vResultDevAddr = vBDevAddr + alignedBufSize;

      // Copy kernel input data to device
      auto vAHostVirtAddr = templ::bit_cast<uint64_t>(vDataAStorageAddKer.back().data());
      streamAddKer.push_back(IDevOpsApiCmd::createCmd<DataWriteCmd>(false, vADevAddr, vAHostVirtAddr,
                                                                    vDataAStorageAddKer.back().size() * sizeof(int),
                                                                    device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      auto vBHostVirtAddr = templ::bit_cast<uint64_t>(vDataBStorageAddKer.back().data());
      streamAddKer.push_back(IDevOpsApiCmd::createCmd<DataWriteCmd>(false, vBDevAddr, vBHostVirtAddr,
                                                                    vDataBStorageAddKer.back().size() * sizeof(int),
                                                                    device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

      // Load ELF
      uint64_t kernelEntryDevAddrAddKer;
      loadElfToDevice(deviceIdxAddKer, readersAddKer[queueIdxAddKer], elfPathAddKer, streamAddKer,
                      kernelEntryDevAddrAddKer);

      // create device params structure
      struct Params {
        uint64_t vA;
        uint64_t vB;
        uint64_t vResult;
        int numElemsAddKer;
      };
      Params parameters = {vADevAddr, vBDevAddr, vResultDevAddr, numElemsAddKer};

      // allocate space for kernel args
      auto kernelArgsDevAddr = getDmaWriteAddr(deviceIdxAddKer, 0x400);

      // allocate 1MB space for kernel error/exception buffer
      auto kernelExceptionDevAddr = getDmaWriteAddr(deviceIdxAddKer, 0x100000);

      // Launch Kernel Command
      streamAddKer.push_back(IDevOpsApiCmd::createCmd<KernelLaunchCmd>(
        true, kernelEntryDevAddrAddKer, kernelArgsDevAddr, kernelExceptionDevAddr, shire_mask, 0,
        templ::bit_cast<uint64_t*>(&parameters), sizeof(parameters), "",
        device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));

      // Read back Kernel Results from device
      vTempDataResultAddKer.resize(numElemsAddKer, 0);
      vResultStorageAddKer.push_back(std::move(vTempDataResultAddKer));
      auto vResultHostVirtAddr = templ::bit_cast<uint64_t>(vResultStorageAddKer.back().data());
      streamAddKer.push_back(
        IDevOpsApiCmd::createCmd<DataReadCmd>(true, vResultDevAddr, vResultHostVirtAddr,
                                              vResultStorageAddKer.back().size() * sizeof(vTempDataResultAddKer[0]),
                                              device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

      // Save off the data
      insertStream(deviceIdxAddKer, queueIdxAddKer, std::move(streamAddKer));
      streamAddKer.clear();
    }
    readersStorageAddKer.push_back(std::move(readersAddKer));
  }

  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // Verify Vector's Data
  for (int deviceIdxAddKer = 0, i = 0; deviceIdxAddKer < deviceCountAddKer; deviceIdxAddKer++) {
    int queueCountAddKer = getSqCount(deviceIdxAddKer);
    for (int queueIdxAddKer = 0; queueIdxAddKer < queueCountAddKer; queueIdxAddKer++) {
      ASSERT_EQ(vSumStorageAddKer[i], vResultStorageAddKer[i])
        << "Vectors result mismatch for device: " << deviceIdxAddKer << ", queue: " << queueIdxAddKer;
      i++;
    }
  }

  deleteStreams();
  TEST_VLOG(1) << "====> ADD TWO VECTORS KERNEL RESPONSE DATA VERIFIED <====" << std::endl;
}

void TestDevOpsApiKernelCmds::launchUberKernel_PositiveTesting_4_4(uint64_t shire_mask) {
  std::vector<CmdTag> streamUberKer;

  std::vector<std::vector<ELFIO::elfio>> readersStorageUberKer;
  std::vector<ELFIO::elfio> readersUberKer;
  auto elfPathUberKer = (fs::path(FLAGS_kernels_dir) / fs::path("uberkernel.elf")).string();

  // Allocate device buffer for layer 0
  const uint64_t numElemsLayer0 = 200;
  const uint64_t bufSizeLayer0 = sizeof(uint64_t) * numElemsLayer0;

  // Allocate device buffer for layer 1
  const uint64_t numElemsLayer1 = 400;
  const uint64_t bufSizeLayer1 = sizeof(uint64_t) * numElemsLayer1;

  std::vector<std::vector<uint64_t>> vResultStorageUberKer;
  std::vector<uint64_t> vResultUberKer;

  // Setup and allocate kernel launch args
  struct layer_parameters_t {
    uint64_t data_ptr;
    uint64_t length;
    uint32_t shire_count;
  };
  std::array<layer_parameters_t, 2> launchArgs;

  int deviceCountUberKer = getDevicesCount();
  for (int deviceIdxUberKer = 0; deviceIdxUberKer < deviceCountUberKer; deviceIdxUberKer++) {
    int queueCountUberKer = getSqCount(deviceIdxUberKer);
    readersUberKer.resize(queueCountUberKer);
    for (uint8_t queueIdxUberKer = 0; queueIdxUberKer < queueCountUberKer; queueIdxUberKer++) {
      auto devAddrBufLayer0 = getDmaWriteAddr(deviceIdxUberKer, bufSizeLayer0);
      auto devAddrBufLayer1 = getDmaWriteAddr(deviceIdxUberKer, bufSizeLayer1);
      TEST_VLOG(1) << "SQ : " << queueIdxUberKer << " devAddrBufLayer0: 0x" << std::hex << devAddrBufLayer0;
      TEST_VLOG(1) << "SQ : " << queueIdxUberKer << " devAddrBufLayer1: 0x" << std::hex << devAddrBufLayer1;
      auto devAddrKernelArgs =
        getDmaWriteAddr(deviceIdxUberKer, ALIGN(sizeof(launchArgs[0]) * launchArgs.size(), kCacheLineSize));
      // allocate 1MB space for kernel error/exception buffer
      auto kernelExceptionDevAddr = getDmaWriteAddr(deviceIdxUberKer, 0x100000);

      launchArgs[0].data_ptr = devAddrBufLayer0;
      launchArgs[0].length = bufSizeLayer0;
      launchArgs[0].shire_count = std::bitset<64>(shire_mask).count();
      launchArgs[1].data_ptr = devAddrBufLayer1;
      launchArgs[1].length = bufSizeLayer1;
      launchArgs[1].shire_count = std::bitset<64>(shire_mask).count();

      uint64_t kernelEntryDevAddrUberKer;
      loadElfToDevice(deviceIdxUberKer, readersUberKer[queueIdxUberKer], elfPathUberKer, streamUberKer,
                      kernelEntryDevAddrUberKer);

      // Kernel launch Command
      streamUberKer.push_back(IDevOpsApiCmd::createCmd<KernelLaunchCmd>(
        device_ops_api::CMD_FLAGS_BARRIER_ENABLE, kernelEntryDevAddrUberKer, devAddrKernelArgs, kernelExceptionDevAddr,
        shire_mask, 0, templ::bit_cast<uint64_t*>(launchArgs.data()), sizeof(launchArgs[0]) * launchArgs.size(), "",
        device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));

      // Read back data written by layer 0
      vResultUberKer.resize(numElemsLayer0, 0xEEEEEEEEEEEEEEEEULL);
      vResultStorageUberKer.push_back(std::move(vResultUberKer));
      auto hostVirtAddr = templ::bit_cast<uint64_t>(vResultStorageUberKer.back().data());
      streamUberKer.push_back(IDevOpsApiCmd::createCmd<DataReadCmd>(
        device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrBufLayer0, hostVirtAddr,
        vResultStorageUberKer.back().size() * sizeof(vResultUberKer[0]),
        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

      // Read back data written by layer 1
      vResultUberKer.resize(numElemsLayer1, 0xEEEEEEEEEEEEEEEEULL);
      vResultStorageUberKer.push_back(std::move(vResultUberKer));
      hostVirtAddr = templ::bit_cast<uint64_t>(vResultStorageUberKer.back().data());
      streamUberKer.push_back(IDevOpsApiCmd::createCmd<DataReadCmd>(
        device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrBufLayer1, hostVirtAddr,
        vResultStorageUberKer.back().size() * sizeof(vResultUberKer[0]),
        device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

      // Save stream against deviceIdx and queueIdx
      insertStream(deviceIdxUberKer, queueIdxUberKer, std::move(streamUberKer));
      streamUberKer.clear();
    }
    readersStorageUberKer.push_back(std::move(readersUberKer));
  }
  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // Verify data
  const std::vector<uint64_t> refdata0(numElemsLayer0, 0xBEEFBEEFBEEFBEEFULL);
  const std::vector<uint64_t> refdata1(numElemsLayer1, 0xBEEFBEEFBEEFBEEFULL);

  for (int deviceIdxUberKer = 0, i = 0; deviceIdxUberKer < deviceCountUberKer; deviceIdxUberKer++) {
    int queueCountUberKer = getSqCount(deviceIdxUberKer);
    for (uint8_t queueIdxUberKer = 0; queueIdxUberKer < queueCountUberKer; queueIdxUberKer++) {
      ASSERT_EQ(vResultStorageUberKer[i * 2], refdata0);
      ASSERT_EQ(vResultStorageUberKer[(i * 2) + 1], refdata1);
      i++;
    }
  }

  deleteStreams();
  TEST_VLOG(1) << "====> UBERKERNEL KERNEL RESPONSE DATA VERIFIED <====" << std::endl;
}

void TestDevOpsApiKernelCmds::launchEmptyKernel_PositiveTesting_4_5(uint64_t shire_mask) {
  std::vector<CmdTag> streamEmptyKer;
  std::vector<std::vector<ELFIO::elfio>> readersStorageEmptyKer;
  std::vector<ELFIO::elfio> readers;
  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path("empty.elf")).string();

  std::vector<unsigned long int> dummyKernelArgs(1, 0xDEADBEEFBEEFDEADULL);
  std::vector<std::vector<uint8_t>> vResultStorage;
  std::vector<uint8_t> vResult;
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    int queueCount = getSqCount(deviceIdx);
    readers.resize(queueCount);
    for (uint8_t queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      // Load ELF
      uint64_t kernelEntryDevAddrEmptyKer;
      loadElfToDevice(deviceIdx, readers[queueIdx], elfPath, streamEmptyKer, kernelEntryDevAddrEmptyKer);
      // allocate space for kernel args
      auto devAddrKernelArgs =
        getDmaWriteAddr(deviceIdx, ALIGN(dummyKernelArgs.size() * sizeof(unsigned long int), kCacheLineSize));
      // allocate 4KB space for dummy kernel result
      auto devAddrKernelResult = getDmaWriteAddr(deviceIdx, ALIGN(sizeof(0x1000), kCacheLineSize));
      // allocate 1MB space for kernel error/exception buffer
      auto kernelExceptionDevAddr = getDmaWriteAddr(deviceIdx, 0x100000);
      // kernel launch
      streamEmptyKer.push_back(IDevOpsApiCmd::createCmd<KernelLaunchCmd>(
        device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kernelEntryDevAddrEmptyKer, devAddrKernelArgs,
        kernelExceptionDevAddr, shire_mask, 0, dummyKernelArgs.data(),
        static_cast<int>(dummyKernelArgs.size() * sizeof(unsigned long int)), "",
        device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));
      // Do a dummy read back Kernel Results from device
      vResult.resize(0x1000, 0);
      vResultStorage.push_back(std::move(vResult));
      auto hostVirtAddr = templ::bit_cast<uint64_t>(vResultStorage.back().data());
      streamEmptyKer.push_back(IDevOpsApiCmd::createCmd<DataReadCmd>(
        device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrKernelResult, hostVirtAddr,
        vResultStorage.back().size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

      // Save stream against deviceIdx and queueIdx
      insertStream(deviceIdx, queueIdx, std::move(streamEmptyKer));
      streamEmptyKer.clear();
    }
    readersStorageEmptyKer.push_back(std::move(readers));
  }
  executeAsync();
  deleteStreams();

  TEST_VLOG(1) << "====> EMPTY KERNEL DONE <====" << std::endl;
}

void TestDevOpsApiKernelCmds::launchExceptionKernel_NegativeTesting_4_6(uint64_t shire_mask) {
  std::vector<CmdTag> streamExceptKer;
  std::vector<std::vector<ELFIO::elfio>> readersStorageExceptKer;
  std::vector<ELFIO::elfio> readersExceptKer;
  auto elfPathExceptKer = (fs::path(FLAGS_kernels_dir) / fs::path("exception.elf")).string();

  std::vector<std::vector<uint8_t>> vResultStorage;
  std::vector<uint8_t> vResult;
  std::vector<device_ops_api::tag_id_t> kernelLaunchTagId;
  int deviceCountExceptKer = getDevicesCount();
  for (int deviceIdxExceptKer = 0; deviceIdxExceptKer < deviceCountExceptKer; deviceIdxExceptKer++) {
    int queueCountExceptKer = getSqCount(deviceIdxExceptKer);
    readersExceptKer.resize(queueCountExceptKer);
    for (uint8_t queueIdxExceptKer = 0; queueIdxExceptKer < queueCountExceptKer; queueIdxExceptKer++) {
      // allocate 1MB space for kernel error/exception buffer
      auto kernelExceptionDevAddr = getDmaWriteAddr(deviceIdxExceptKer, 0x100000);
      uint64_t kernelEntryDevAddrExceptKer;
      loadElfToDevice(deviceIdxExceptKer, readersExceptKer[queueIdxExceptKer], elfPathExceptKer, streamExceptKer,
                      kernelEntryDevAddrExceptKer);
      // Kernel launch
      streamExceptKer.push_back(IDevOpsApiCmd::createCmd<KernelLaunchCmd>(
        device_ops_api::CMD_FLAGS_BARRIER_ENABLE, kernelEntryDevAddrExceptKer, 0 /* No kernel args */,
        kernelExceptionDevAddr, shire_mask, 0, nullptr, 0, "exception.elf",
        device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_EXCEPTION));
      kernelLaunchTagId.push_back(streamExceptKer.back());

      // pull the exception buffer from device
      vResult.resize(0x1000, 0);
      vResultStorage.push_back(std::move(vResult));
      auto hostVirtAddr = templ::bit_cast<uint64_t>(vResultStorage.back().data());
      streamExceptKer.push_back(IDevOpsApiCmd::createCmd<DataReadCmd>(
        device_ops_api::CMD_FLAGS_BARRIER_ENABLE, kernelExceptionDevAddr, hostVirtAddr,
        vResultStorage.back().size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

      // Save stream against deviceIdx and queueIdx
      insertStream(deviceIdxExceptKer, queueIdxExceptKer, std::move(streamExceptKer));
      streamExceptKer.clear();
    }
    readersStorageExceptKer.push_back(std::move(readersExceptKer));
  }
  executeAsync();

  // print the exception buffer for the first two shires
  for (int deviceIdxExceptKer = 0, i = 0; deviceIdxExceptKer < deviceCountExceptKer; deviceIdxExceptKer++) {
    int queueCountExceptKer = getSqCount(deviceIdxExceptKer);
    for (int queueIdxExceptKer = 0; queueIdxExceptKer < queueCountExceptKer; queueIdxExceptKer++) {
      printErrorContext(queueIdxExceptKer, templ::bit_cast<std::byte*>(vResultStorage[i].data()), 0x3,
                        kernelLaunchTagId[i]);
      i++;
    }
  }

  deleteStreams();
  TEST_VLOG(1) << "====> EXCEPTION KERNEL DONE <====" << std::endl;
}

void TestDevOpsApiKernelCmds::launchHangKernel(uint64_t shire_mask, bool sendAbortCmd) {
  std::vector<CmdTag> streamHangKer;
  std::vector<std::vector<ELFIO::elfio>> readersStorageHangKer;
  std::vector<ELFIO::elfio> readersHangKer;
  auto elfPathHangKer = (fs::path(FLAGS_kernels_dir) / fs::path("hang.elf")).string();

  std::vector<std::vector<uint8_t>> vResultStorageHangKer;
  std::vector<uint8_t> vResultHangKer;
  std::vector<device_ops_api::tag_id_t> kernelLaunchTagId;
  int tagIdIdx = 0;
  int deviceCountHangKer = getDevicesCount();
  device_ops_api::dev_ops_api_kernel_launch_response_e hangKerExpectedRsp;
  if (sendAbortCmd) {
    hangKerExpectedRsp = device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED;
  } else {
    hangKerExpectedRsp = device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_TIMEOUT_HANG;
  }
  for (int deviceIdxHangKer = 0; deviceIdxHangKer < deviceCountHangKer; deviceIdxHangKer++) {
    int queueCountHangKer = getSqCount(deviceIdxHangKer);
    readersHangKer.resize(queueCountHangKer);
    for (uint8_t queueIdxHangKer = 0; queueIdxHangKer < queueCountHangKer; queueIdxHangKer++) {
      // allocate 1MB space for kernel error/exception buffer
      auto devAddrkernelException = getDmaWriteAddr(deviceIdxHangKer, 0x100000);
      uint64_t kernelEntryDevAddrHangKer;
      loadElfToDevice(deviceIdxHangKer, readersHangKer[queueIdxHangKer], elfPathHangKer, streamHangKer,
                      kernelEntryDevAddrHangKer);
      // Kernel launch
      streamHangKer.push_back(IDevOpsApiCmd::createCmd<KernelLaunchCmd>(
        device_ops_api::CMD_FLAGS_BARRIER_ENABLE, kernelEntryDevAddrHangKer, 0 /* No kernel args */,
        devAddrkernelException, shire_mask, 0, nullptr, 0, "hang.elf", hangKerExpectedRsp));
      kernelLaunchTagId.push_back(streamHangKer.back());

      if (sendAbortCmd) {
        // Kernel Abort
        streamHangKer.push_back(IDevOpsApiCmd::createCmd<KernelAbortCmd>(
          device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kernelLaunchTagId[tagIdIdx],
          device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS));
        tagIdIdx++;
      }
      // pull the exception buffer from device
      vResultHangKer.resize(0x100000, 0);
      vResultStorageHangKer.push_back(std::move(vResultHangKer));
      auto hostVirtAddr = templ::bit_cast<uint64_t>(vResultStorageHangKer.back().data());
      streamHangKer.push_back(IDevOpsApiCmd::createCmd<DataReadCmd>(
        device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrkernelException, hostVirtAddr,
        vResultStorageHangKer.back().size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

      // Save stream against deviceIdx and queueIdx
      insertStream(deviceIdxHangKer, queueIdxHangKer, std::move(streamHangKer));
      streamHangKer.clear();
    }
    readersStorageHangKer.push_back(std::move(readersHangKer));
  }
  executeAsync();

  // print the exception buffer for the first two shires
  for (int deviceIdxHangKer = 0, i = 0; deviceIdxHangKer < deviceCountHangKer; deviceIdxHangKer++) {
    int queueCountHangKer = getSqCount(deviceIdxHangKer);
    for (int queueIdxHangKer = 0; queueIdxHangKer < queueCountHangKer; queueIdxHangKer++) {
      printErrorContext(queueIdxHangKer, templ::bit_cast<std::byte*>(vResultStorageHangKer[i].data()), 0x3,
                        kernelLaunchTagId[i]);
      i++;
    }
  }
  deleteStreams();

  TEST_VLOG(1) << "====> HANG KERNEL DONE <====" << std::endl;
}

void TestDevOpsApiKernelCmds::kernelAbortCmd_InvalidTagIdNegativeTesting_6_2() {
  std::vector<CmdTag> streamAbortCmd;
  int deviceCountAbortCmd = getDevicesCount();
  for (int deviceIdxAbortCmd = 0; deviceIdxAbortCmd < deviceCountAbortCmd; deviceIdxAbortCmd++) {
    auto queueCountAbortCmd = getSqCount(deviceIdxAbortCmd);
    for (uint8_t queueIdxAbortCmd = 0; queueIdxAbortCmd < queueCountAbortCmd; queueIdxAbortCmd++) {
      streamAbortCmd.push_back(
        IDevOpsApiCmd::createCmd<KernelAbortCmd>(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, 0xbeef /* invalid tagId */,
                                                 device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_INVALID_TAG_ID));

      // Save stream against deviceIdx and queueIdx[queueIdx]
      insertStream(deviceIdxAbortCmd, queueIdxAbortCmd, std::move(streamAbortCmd));
      streamAbortCmd.clear();
    }
  }
  executeAsync();
  deleteStreams();
}

/**********************************************************
 *                                                         *
 *                  Kernel Stress Tests                    *
 *                                                         *
 **********************************************************/

void TestDevOpsApiKernelCmds::varifyAddKernelLaunchKernel(bool singleDevice, bool singleQueue,
      const std::vector<std::vector<int>> sumOfVectorAB,
      const std::vector<std::vector<int>> resultOfVectorAB,
      std::unordered_map<size_t, int> kerCountStorage) {

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
      return;
    }

  // Verify Add kernel Vector's Data
  auto deviceCount = singleDevice ? 1 : getDevicesCount();
  for (int deviceIdx = 0, i = 0; deviceIdx < deviceCount; deviceIdx++) {
    auto queueCount = singleQueue ? 1 : getSqCount(deviceIdx);
    for (int queueIdx = 0; queueIdx < queueCount; queueIdx++) {
      for (int kernelIdx = 0; kernelIdx < kerCountStorage[key(deviceIdx, queueIdx)]; kernelIdx++) {
        ASSERT_EQ(sumOfVectorAB[i], resultOfVectorAB[i])
                << "Vectors result mismatch for device: " << deviceIdx << ", queue: " << queueIdx
                << " at Index: " << i;
        i++;
      }
    }
  }
}

void TestDevOpsApiKernelCmds::backToBackSameKernelLaunchCmds_3_1(bool singleDevice, bool singleQueue, uint64_t totalKer,
                                                                 uint64_t shire_mask) {
  std::vector<CmdTag> streamSameKer;
  std::vector<std::vector<ELFIO::elfio>> readersStorageSameKer;
  std::vector<ELFIO::elfio> readersSameKer;
  auto elfPathSameKer = (fs::path(FLAGS_kernels_dir) / fs::path("add_vector.elf")).string();

  int numElemsSameKer = 4 * 1024;
  auto bufSize = numElemsSameKer * sizeof(int);
  auto alignedBufSize = ALIGN(bufSize, kCacheLineSize);
  // Create vector data
  std::vector<std::vector<int>> vDataAStorageSameKer;
  std::vector<std::vector<int>> vDataBStorageSameKer;
  std::vector<std::vector<int>> vSumStorageSameKer;
  std::vector<std::vector<int>> vResultStorageSameKer;
  std::vector<int> vTempDataResultSameKer;
  std::unordered_map<size_t, int> kerCountStorage;
  // create device params structure
  struct Params {
    uint64_t vASameKer;
    uint64_t vBSameKer;
    uint64_t vResultSameKer;
    int numElementsSameKer;
  };
  auto deviceCountSameKer = singleDevice ? 1 : getDevicesCount();
  int kernelCount = 0;
  for (int deviceIdxSameKer = 0; deviceIdxSameKer < deviceCountSameKer; deviceIdxSameKer++) {
    auto queueCountSameKer = singleQueue ? 1 : getSqCount(deviceIdxSameKer);
    readersSameKer.resize(totalKer);
    for (uint16_t queueIdxSameKer = 0; queueIdxSameKer < queueCountSameKer; queueIdxSameKer++) {
      std::vector<uint64_t> devAddrVecResultPerQueue;
      for (int i = 0; i < totalKer / deviceCountSameKer/  queueCountSameKer; i++) {
        generateRandomData(numElemsSameKer, vDataAStorageSameKer, vDataBStorageSameKer, vSumStorageSameKer);

        auto hostVirtAddrVecA = templ::bit_cast<uint64_t>(vDataAStorageSameKer.back().data());
        auto hostVirtAddrVecB = templ::bit_cast<uint64_t>(vDataBStorageSameKer.back().data());
        // Load kernel ELF
        uint64_t kernelEntryDevAddrSameKer;
        loadElfToDevice(deviceIdxSameKer, readersSameKer[kernelCount], elfPathSameKer, streamSameKer,
                        kernelEntryDevAddrSameKer);
        // Copy kernel input data to device
        auto dataLoadAddr = getDmaWriteAddr(deviceIdxSameKer, 2 * alignedBufSize);
        auto devAddrVecA = dataLoadAddr;
        auto devAddrVecB = devAddrVecA + alignedBufSize;
        streamSameKer.push_back(IDevOpsApiCmd::createCmd<DataWriteCmd>(
          device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devAddrVecA, hostVirtAddrVecA,
          vDataAStorageSameKer.back().size() * sizeof(int), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
        streamSameKer.push_back(IDevOpsApiCmd::createCmd<DataWriteCmd>(
          device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devAddrVecB, hostVirtAddrVecB,
          vDataBStorageSameKer.back().size() * sizeof(int), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

        // allocate space for kernel args
        devAddrVecResultPerQueue.push_back(getDmaWriteAddr(deviceIdxSameKer, alignedBufSize));
        auto devAddrKernelArgs = getDmaWriteAddr(deviceIdxSameKer, ALIGN(sizeof(Params), kCacheLineSize));
        // allocate 1MB space for kernel error/exception buffer
        auto devAddrkernelException = getDmaWriteAddr(deviceIdxSameKer, 0x100000);

        Params kerParams = {devAddrVecA, devAddrVecB, devAddrVecResultPerQueue.back(), numElemsSameKer};
        // Launch Kernel Command
        streamSameKer.push_back(IDevOpsApiCmd::createCmd<KernelLaunchCmd>(
          device_ops_api::CMD_FLAGS_BARRIER_ENABLE, kernelEntryDevAddrSameKer, devAddrKernelArgs,
          devAddrkernelException, shire_mask, 0, templ::bit_cast<uint64_t*>(&kerParams), sizeof(kerParams), "",
          device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));
        kernelCount++;
      }

      kerCountStorage.try_emplace(key(deviceIdxSameKer, queueIdxSameKer), devAddrVecResultPerQueue.size());
      // Read back Kernel Results from device
      for (int i = 0; i < devAddrVecResultPerQueue.size(); i++) {
        vTempDataResultSameKer.resize(numElemsSameKer, 0);
        vResultStorageSameKer.push_back(std::move(vTempDataResultSameKer));
        auto hostVirtAddrRes = templ::bit_cast<uint64_t>(vResultStorageSameKer.back().data());
        streamSameKer.push_back(IDevOpsApiCmd::createCmd<DataReadCmd>(
          (i == 0) ? device_ops_api::CMD_FLAGS_BARRIER_ENABLE
                   : device_ops_api::CMD_FLAGS_BARRIER_DISABLE, /* Barrier only for first read to make sure that all
                                                                   kernels execution done */
                     devAddrVecResultPerQueue[i], hostVirtAddrRes,
          vResultStorageSameKer.back().size() * sizeof(vTempDataResultSameKer[0]),
          device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      }
      // Save stream against deviceIdx and queueIdx
      insertStream(deviceIdxSameKer, queueIdxSameKer, std::move(streamSameKer));
      streamSameKer.clear();
    }
    readersStorageSameKer.push_back(std::move(readersSameKer));
  }

  executeAsync();

  // Verify kernel's data
  varifyAddKernelLaunchKernel(singleDevice, singleQueue, vSumStorageSameKer, vResultStorageSameKer, kerCountStorage);

  deleteStreams();

  TEST_VLOG(1) << "====> BACK TO BACK " << totalKer << " KERNEL LAUNCH (ADD VECTORS KERNEL) DATA VERIFIED <====\n"
               << std::endl;
}

void TestDevOpsApiKernelCmds::backToBackDifferentKernelLaunchCmds_3_2(bool singleDevice, bool singleQueue,
                                                                      uint64_t totalKer, uint64_t shire_mask) {
  int numElemsDiffKer = 1024;
  std::vector<KernelTypes> kerTypes = generateKernelTypes(KernelTypes::ADD_KERNEL_TYPE, totalKer, 0x3);
  kernelContainer_t kernels = buildKernelsInfo(kerTypes, singleDevice, singleQueue, false);

  /*******************************************************
   *                  load all ELF files                 *
   * *****************************************************/
  std::vector<std::vector<ELFIO::elfio>> readersStorageDiffKer;
  // Load add_vector ELF kerTypes = 0
  std::vector<ELFIO::elfio> readersDiffKer;
  auto addKerElfPath = (fs::path(FLAGS_kernels_dir) / fs::path("add_vector.elf")).string();
  // Load exception ELF kerTypes = 1
  auto excepKerElfPath = (fs::path(FLAGS_kernels_dir) / fs::path("exception.elf")).string();
  // Load hang ELF kerTypes = 2
  auto hangKerElfPath = (fs::path(FLAGS_kernels_dir) / fs::path("hang.elf")).string();

  /*******************************************************
   *                  Setup for Add Kernel               *
   * *****************************************************/
  // Create vector data
  std::vector<std::vector<int>> vDataAStorageDiffKer;
  std::vector<std::vector<int>> vDataBStorageDiffKer;
  std::vector<std::vector<int>> vSumStorageDiffKer;
  std::vector<std::vector<int>> vResultStorageDiffKer;
  std::vector<int> vTempDataResultDiffKer;
  // create device params structure
  struct Params {
    uint64_t dataA;
    uint64_t dataB;
    uint64_t dataResult;
    int numElements;
  };
  Params addKerParams;
  int kerCount = 0;
  // Assign device memory for data - a hard coded hack for now
  auto bufSize = numElemsDiffKer * sizeof(int);
  auto alignedBufSize = ALIGN(bufSize, kCacheLineSize);
  std::unordered_map<size_t, int> kerCountStorage;
  auto deviceCountDiffKer = singleDevice ? 1 : getDevicesCount();

  for (const auto& perQueueKernels : kernels) {
    std::vector<CmdTag> streamDiffKer;
    auto deviceIdxDiffKer = perQueueKernels.deviceIdx;
    auto queueIdxDiffKer = perQueueKernels.queueIdx;
    std::vector<uint64_t> devAddrVecResultPerQueue;
    readersDiffKer.resize(totalKer);
    for (auto kerType : perQueueKernels.kernels) {
      device_ops_api::tag_id_t kernelLaunchTagId;
      switch (kerType) {
      case KernelTypes::ADD_KERNEL_TYPE: /* Add Kernel */ {
        generateRandomData(numElemsDiffKer, vDataAStorageDiffKer, vDataBStorageDiffKer, vSumStorageDiffKer);
        uint64_t addKernelEntryAddrDiffKer;
        loadElfToDevice(deviceIdxDiffKer, readersDiffKer[kerCount], addKerElfPath, streamDiffKer,
            addKernelEntryAddrDiffKer);
        // Allocate device space for vector A, B and resultant vectors (totalAddKer)
        auto dataLoadAddr = getDmaWriteAddr(deviceIdxDiffKer, 2 * alignedBufSize);
        auto devAddrVecA = dataLoadAddr;
        auto devAddrVecB = devAddrVecA + alignedBufSize;
        // Copy kernel input data to device
        auto hostVirtAddrA = templ::bit_cast<uint64_t>(vDataAStorageDiffKer.back().data());
        streamDiffKer.push_back(IDevOpsApiCmd::createCmd<DataWriteCmd>(
            device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devAddrVecA, hostVirtAddrA,
            vDataAStorageDiffKer.back().size() * sizeof(int), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
        auto hostVirtAddrB = templ::bit_cast<uint64_t>(vDataBStorageDiffKer.back().data());
        streamDiffKer.push_back(IDevOpsApiCmd::createCmd<DataWriteCmd>(
            device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devAddrVecB, hostVirtAddrB,
            vDataBStorageDiffKer.back().size() * sizeof(int), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

        // Copy kernel args to device
        devAddrVecResultPerQueue.push_back(getDmaWriteAddr(deviceIdxDiffKer, alignedBufSize));
        addKerParams = {devAddrVecA, devAddrVecB, devAddrVecResultPerQueue.back(), numElemsDiffKer};
        auto devAddrKernelArgs = getDmaWriteAddr(deviceIdxDiffKer, ALIGN(sizeof(Params), kCacheLineSize));
        // allocate 1MB space for kernel error/exception buffer
        auto devAddrkernelException = getDmaWriteAddr(deviceIdxDiffKer, 0x100000);

        // Launch Kernel Command for add kernel
        streamDiffKer.push_back(IDevOpsApiCmd::createCmd<KernelLaunchCmd>(
            device_ops_api::CMD_FLAGS_BARRIER_ENABLE, addKernelEntryAddrDiffKer, devAddrKernelArgs,
            devAddrkernelException, shire_mask, 0, templ::bit_cast<uint64_t*>(&addKerParams), sizeof(Params), "",
            device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));
        kerCount++;
        break;
      }
      case KernelTypes::EXCEP_KERNEL_TYPE: {
        uint64_t excepKerEntryAddr;
        loadElfToDevice(deviceIdxDiffKer, readersDiffKer[kerCount], excepKerElfPath, streamDiffKer,
            excepKerEntryAddr);
        // allocate 1MB space for kernel error/exception buffer
        auto devAddrkernelException = getDmaWriteAddr(deviceIdxDiffKer, 0x100000);
        // Launch Kernel Command for exception kernel
        streamDiffKer.push_back(IDevOpsApiCmd::createCmd<KernelLaunchCmd>(
            device_ops_api::CMD_FLAGS_BARRIER_ENABLE, excepKerEntryAddr, 0 /* No kernel args */, devAddrkernelException,
            shire_mask, 0, templ::bit_cast<uint64_t*>(&addKerParams), sizeof(Params), "",
            device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_EXCEPTION));
        kerCount++;
        break;
      }
      case KernelTypes::HANG_KERNEL_TYPE: {
        uint64_t hangKerEntryAddr;
        loadElfToDevice(deviceIdxDiffKer, readersDiffKer[kerCount], hangKerElfPath, streamDiffKer, hangKerEntryAddr);
        // allocate 1MB space for kernel error/exception buffer
        auto devAddrkernelException = getDmaWriteAddr(deviceIdxDiffKer, 0x100000);
        // Kernel launch
        streamDiffKer.push_back(IDevOpsApiCmd::createCmd<KernelLaunchCmd>(
            device_ops_api::CMD_FLAGS_BARRIER_ENABLE, hangKerEntryAddr, 0 /* No kernel args */, devAddrkernelException,
            shire_mask, 0, nullptr, 0, "", device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED));
        kernelLaunchTagId = streamDiffKer.back();
        // Kernel Abort
        streamDiffKer.push_back(
            IDevOpsApiCmd::createCmd<KernelAbortCmd>(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kernelLaunchTagId,
                device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS));
        kerCount++;
        break;
      }
      default:
        break;
      }
    }
    kerCountStorage.try_emplace(key(deviceIdxDiffKer, queueIdxDiffKer), devAddrVecResultPerQueue.size());
    // Read back add Kernel Results from device
    for (int i = 0; i < devAddrVecResultPerQueue.size(); i++) {
      vTempDataResultDiffKer.resize(numElemsDiffKer, 0);
      vResultStorageDiffKer.push_back(std::move(vTempDataResultDiffKer));
      auto hostVirtAddrRes = templ::bit_cast<uint64_t>(vResultStorageDiffKer.back().data());
      streamDiffKer.push_back(IDevOpsApiCmd::createCmd<DataReadCmd>(
          (i == 0) ? device_ops_api::CMD_FLAGS_BARRIER_ENABLE
              : device_ops_api::CMD_FLAGS_BARRIER_DISABLE, /* Barrier only for first read to make sure that all
                                                                   kernels execution done */
                devAddrVecResultPerQueue[i], hostVirtAddrRes,
                vResultStorageDiffKer.back().size() * sizeof(vTempDataResultDiffKer[0]),
                device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
    }
    // Save stream against deviceIdx and queueIdx
    insertStream(deviceIdxDiffKer, queueIdxDiffKer, std::move(streamDiffKer));
    readersStorageDiffKer.push_back(std::move(readersDiffKer));
  }

  executeAsync();

  // Verify kernel's data
  varifyAddKernelLaunchKernel(singleDevice, singleQueue, vSumStorageDiffKer, vResultStorageDiffKer, kerCountStorage);

  deleteStreams();

  TEST_VLOG(1) << "====> BACK TO BACK " << totalKer << " DIFFERENT KERNEL LAUNCH VERIFIED" << std::endl;
}

void TestDevOpsApiKernelCmds::backToBackEmptyKernelLaunch_3_3(uint64_t totalKer, uint64_t shire_mask, bool flushL3) {
  std::vector<CmdTag> streamEmptyKer;
  std::vector<std::vector<ELFIO::elfio>> readersStorageEmptyKer;
  std::vector<ELFIO::elfio> readersEmptyKer;
  auto elfPathEmptyKer = (fs::path(FLAGS_kernels_dir) / fs::path("empty.elf")).string();

  std::vector<std::vector<uint8_t>> vResultStorageEmptyKer;
  std::vector<uint8_t> vResultEmptyKer;
  std::array<uint8_t, 8> dummyKernelArgs = {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF};
  auto deviceCountEmptyKer = getDevicesCount();
  for (int deviceIdxEmptyKer = 0; deviceIdxEmptyKer < deviceCountEmptyKer; deviceIdxEmptyKer++) {
    auto queueCountEmptyKer = getSqCount(deviceIdxEmptyKer);
    readersEmptyKer.resize(totalKer);
    for (uint16_t queueIdxEmptyKer = 0; queueIdxEmptyKer < queueCountEmptyKer; queueIdxEmptyKer++) {
      std::vector<uint64_t> devAddrResult(totalKer / queueCountEmptyKer);
      // Load kernel ELF
      uint64_t kernelEntryAddr;
      loadElfToDevice(deviceIdxEmptyKer, readersEmptyKer[queueIdxEmptyKer], elfPathEmptyKer, streamEmptyKer,
                      kernelEntryAddr);
      for (int i = 0; i < totalKer / queueCountEmptyKer; i++) {
        // allocate 4KB space for dummy kernel result
        devAddrResult[i] = getDmaWriteAddr(deviceIdxEmptyKer, ALIGN(0x1000, kCacheLineSize));
        // allocate space for kernel args
        auto devAddrKernelArgs = getDmaWriteAddr(deviceIdxEmptyKer, ALIGN(sizeof(dummyKernelArgs), kCacheLineSize));
        // allocate 1MB space for kernel error/exception buffer
        auto devAddrkernelException = getDmaWriteAddr(deviceIdxEmptyKer, 0x100000);

        // Launch Kernel Command
        streamEmptyKer.push_back(IDevOpsApiCmd::createCmd<KernelLaunchCmd>(
          (device_ops_api::CMD_FLAGS_BARRIER_ENABLE | (flushL3 ? device_ops_api::CMD_FLAGS_KERNEL_LAUNCH_FLUSH_L3 : 0)),
          kernelEntryAddr, devAddrKernelArgs, devAddrkernelException, shire_mask, 0,
          templ::bit_cast<uint64_t*>(dummyKernelArgs.data()), sizeof(dummyKernelArgs[0]) * dummyKernelArgs.size(), "",
          device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));
      }

      // Read back dummy Kernel Results from device
      for (int i = 0; i < totalKer / queueCountEmptyKer; i++) {
        vResultEmptyKer.resize(0x1000, 0);
        vResultStorageEmptyKer.push_back(std::move(vResultEmptyKer));
        auto hostVirtAddrRes = templ::bit_cast<uint64_t>(vResultStorageEmptyKer.back().data());
        streamEmptyKer.push_back(IDevOpsApiCmd::createCmd<DataReadCmd>(
          i == 0, /* Barrier only for first read to make sure that all kernels execution done */
          devAddrResult[i], hostVirtAddrRes, vResultStorageEmptyKer.back().size(),
          device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      }

      // Save stream against deviceIdx and queueIdx
      insertStream(deviceIdxEmptyKer, queueIdxEmptyKer, std::move(streamEmptyKer));
      streamEmptyKer.clear();
    }
    readersStorageEmptyKer.push_back(std::move(readersEmptyKer));
  }

  executeAsync();
  deleteStreams();

  TEST_VLOG(1) << "====> BACK TO BACK " << totalKer << " KERNEL LAUNCH (EMPTY KERNEL) DONE <====\n" << std::endl;
}

/**********************************************************
*                                                         *
*          Kernel DMA LIST Functions                      *
*                                                         *
**********************************************************/

std::map<KernelTypes, std::string> TestDevOpsApiKernelCmds::kernelELFNames_ = {
    { KernelTypes::ADD_KERNEL_TYPE, "add_vector.elf" },
    { KernelTypes::EXCEP_KERNEL_TYPE, "exception.elf" },
    { KernelTypes::HANG_KERNEL_TYPE, "hang.elf" },
    { KernelTypes::UBER_KERNEL_TYPE, "uberkernel.elf" },
    { KernelTypes::EMPTY_KERNEL_TYPE, "empty.elf" },
    { KernelTypes::CMUMODE_KERNEL_TYPE, "cm_umode_test.elf" },
};

device_ops_api::dma_write_node
TestDevOpsApiKernelCmds::fillDMAWriteNode(uint64_t srcHostVirtAddr, uint64_t dstDevPhyAddr, uint32_t size) const {
  device_ops_api::dma_write_node node;

  node.src_host_virt_addr = srcHostVirtAddr;
  node.dst_device_phy_addr = dstDevPhyAddr;
  node.size = size;
  return node;
}

device_ops_api::dma_read_node
TestDevOpsApiKernelCmds::fillDMAReadNode(uint64_t dstHostVirtAddr, uint64_t srcDevPhyAddr, uint32_t size) const {
  device_ops_api::dma_read_node node;

  node.dst_host_virt_addr = dstHostVirtAddr;
  node.src_device_phy_addr = srcDevPhyAddr;
  node.size = size;
  return node;
}

uint64_t
TestDevOpsApiKernelCmds::loadElf(int deviceIdx, KernelTypes kerType, device_ops_api::dma_write_node& node) {
  ELFIO::elfio reader;
  uint64_t kernelEntryAddr;

  auto elfPath = (fs::path(FLAGS_kernels_dir) / fs::path(kernelELFNames_[kerType])).string();

  if (!reader.load(elfPath))
    throw Exception("Uanble to load elf file");

  // We only allow ELFs with 1 segment, and the code should be relocatable.
  // TODO: Properly generate static PIE ELFs
  if (reader.segments.size() != 1)
    throw Exception("File segment size is not equal to 1");

  const segment *segment0 = reader.segments[0];
  if (!(segment0->get_type() & PT_LOAD))
    throw Exception("File segment type is not PT_LOAD");

  // Copy segment to device memory
  auto entry = reader.get_entry();
  auto vAddr = segment0->get_virtual_address();
  auto pAddr = segment0->get_physical_address();
  auto fileSize = segment0->get_file_size();
  auto memSize = segment0->get_memory_size();

  TEST_VLOG(1)<< std::endl << "Loading ELF: " << elfPath << std::endl
      << "  ELF Entry: 0x" << std::hex << entry
      << "  Segment [0]: "
      << "vAddr: 0x" << std::hex << vAddr << ", pAddr: 0x" << std::hex << pAddr << ", Mem Size: 0x" << memSize
      << ", File Size: 0x" << fileSize;

  // Allocate device buffer for the ELF segment
  uint64_t deviceElfSegment0Buffer = getDmaWriteAddr(deviceIdx, ALIGN(memSize, 0x1000));
  kernelEntryAddr = deviceElfSegment0Buffer + (entry - vAddr);

  TEST_VLOG(1)<< " Allocated buffer at device address: 0x" << deviceElfSegment0Buffer << " for segment 0"
      << " Kernel entry at device address: 0x" << kernelEntryAddr;

  // Fill DMA write list node
  auto fileData = static_cast<char*>(static_cast<void*>(getMmapBuffer(deviceIdx, ALIGN(fileSize, 0x1000))));
  memcpy(fileData, segment0->get_data(), fileSize);
  node =  fillDMAWriteNode(templ::bit_cast<uint64_t>(fileData), deviceElfSegment0Buffer, static_cast<uint32_t>(fileSize));

  return kernelEntryAddr;
}

std::vector<KernelTypes> TestDevOpsApiKernelCmds::generateKernelTypes (
    KernelTypes kernelType, uint64_t totalKernel, int numKernelTypes) const {
  std::vector<KernelTypes> kerTypes;

  if (numKernelTypes > static_cast<int>(KernelTypes::EMPTY_KERNEL_TYPE)) {
    numKernelTypes = static_cast<int>(KernelTypes::EMPTY_KERNEL_TYPE);
  }

  std::minstd_rand simpleRand;
  simpleRand.seed(time(nullptr));
  for (auto i = 0; i < totalKernel; ++i) {
    if (numKernelTypes == 1)
      kerTypes.push_back(kernelType);
    else {
      int val = simpleRand() % numKernelTypes;
      kerTypes.push_back(static_cast<KernelTypes>(val));
    }
  }

  return kerTypes;
}

uint64_t
TestDevOpsApiKernelCmds::kernelExceptionSpace(int deviceIdx) {
  return getDmaWriteAddr(deviceIdx, kExceptionSize);
}

void
TestDevOpsApiKernelCmds::validataAddKernel(std::vector<AddKerInfo> ExpectedResultAB,
    std::vector<int*> addKernelResultAB) const {

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  for (int i = 0; i < ExpectedResultAB.size(); ++i) {
      ASSERT_EQ(memcmp(ExpectedResultAB[i].data.data(), addKernelResultAB[i], addKerNumElems_ * sizeof(int)), 0)
          << "Vectors result mismatch for device: " << ExpectedResultAB[i].devIdx
          << ", queue: " << ExpectedResultAB[i].queueIdx
          << " at Index: " << i;
    }

  TEST_VLOG(1) << "====> ADD TWO VECTORS KERNEL RESPONSE DATA VERIFIED <====" << std::endl;
}

void
TestDevOpsApiKernelCmds::addKernelResultReadBackPerQueue(int deviceIdx,
    std::vector<int*>& addKernelResultsVector,
    std::vector<uint64_t> perQueueResultDevAddrs,
    std::vector<CmdTag> &stream) {

  auto bufSize = addKerNumElems_ * sizeof(int);
  auto alignedBufSize = ALIGN(bufSize, kCacheLineSize);
  std::vector<device_ops_api::dma_read_node> readNodes;

  for (size_t i = 0 ; i < perQueueResultDevAddrs.size(); ++i) {
    auto readPtr = static_cast<void*>(getMmapBuffer(deviceIdx, alignedBufSize, false));

    // save result vector
    addKernelResultsVector.push_back(static_cast<int*>(readPtr));
    readNodes.push_back(fillDMAReadNode(templ::bit_cast<uint64_t>(readPtr), perQueueResultDevAddrs[i], static_cast<uint32_t>(bufSize)));

    if (readNodes.size() == 4 || i == perQueueResultDevAddrs.size() - 1) {
      // create DMA read list command
      stream.push_back(
          IDevOpsApiCmd::createCmd<DmaReadListCmd>(
              i < 4 ? // Barrier only for first read to make sure that all kernels execution done
              device_ops_api::CMD_FLAGS_BARRIER_ENABLE : device_ops_api::CMD_FLAGS_BARRIER_DISABLE,
              readNodes.data(), static_cast<uint32_t>(readNodes.size()),
              device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      readNodes.clear();
    }
  }
}

uint64_t
TestDevOpsApiKernelCmds::handleAddKernelDMAListCmd(int deviceIdx,
    uint64_t shireMask, std::vector<int> &sumOfAB,
    std::vector<CmdTag> &stream) {

  auto bufSize = addKerNumElems_ * sizeof(int);
  auto alignedBufSize = ALIGN(bufSize, kCacheLineSize);
  std::vector<device_ops_api::dma_write_node> wrNodes(3);

  auto vectorA = static_cast<int*>(static_cast<void*>(getMmapBuffer(deviceIdx, 2 * alignedBufSize)));
  auto vectorB = vectorA + alignedBufSize / sizeof(int);

  // generate random data
  sumOfAB = addKernelRandomData(addKerNumElems_, vectorA, vectorB);

  // load elf
  auto kernelEntryAddr = loadElf(deviceIdx, addKernelType_, wrNodes[0]);

  auto vectorADevAddr = getDmaWriteAddr(deviceIdx, 3 * alignedBufSize);
  auto vectorBDevAddr = vectorADevAddr + alignedBufSize;
  auto kernelResultAddr = vectorBDevAddr + alignedBufSize;

  // DMA node for vector A
  wrNodes[1] = fillDMAWriteNode(templ::bit_cast<uint64_t>(vectorA), vectorADevAddr, static_cast<uint32_t>(bufSize));

  // DMA node for vector B
  wrNodes[2] = fillDMAWriteNode(templ::bit_cast<uint64_t>(vectorB), vectorBDevAddr, static_cast<uint32_t>(bufSize));

  // add kernel parameters
  AddKerParams_t params = { vectorADevAddr, vectorBDevAddr,  kernelResultAddr, addKerNumElems_ };
  // alocate space for argument
  auto devKernelArgsAddr = getDmaWriteAddr(deviceIdx, ALIGN(sizeof(AddKerParams_t), kCacheLineSize));

  // create DMA write list command
  stream.push_back(
      IDevOpsApiCmd::createCmd<DmaWriteListCmd>(
          device_ops_api::CMD_FLAGS_BARRIER_DISABLE, wrNodes.data(), wrNodes.size(),
          device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // Add kernel launch command
  stream.push_back(
      IDevOpsApiCmd::createCmd<KernelLaunchCmd>(
          device_ops_api::CMD_FLAGS_BARRIER_ENABLE, kernelEntryAddr,
          devKernelArgsAddr, kernelExceptionSpace(deviceIdx), shireMask, 0,
          templ::bit_cast<uint64_t*>(&params), sizeof(params), "",
          device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));

  return kernelResultAddr;
}

void TestDevOpsApiKernelCmds::printExceptionContext(const std::vector<ExcepContextInfo> exceptionContexts) const {
  for (auto context : exceptionContexts)
    printErrorContext(context.queueIdx,
        templ::bit_cast<std::byte*>(context.data), exceptionShireMask_,
        context.tagId);
}

void
TestDevOpsApiKernelCmds::handleExceptionOrAbortKernelDMAListCmd(
    int deviceIdx,int queueIdx, uint64_t shireMask, KernelTypes kertype, std::vector<ExcepContextInfo>& contexts,
    std::vector<CmdTag> &stream) {

  ExcepContextInfo info;
  std::vector<device_ops_api::dma_write_node> wrNodes(1);

  // load elf
  auto kernelEntryAddr = loadElf(deviceIdx, kertype, wrNodes[0]);

  // create DMA write list command
  stream.push_back(
      IDevOpsApiCmd::createCmd<DmaWriteListCmd>(
          device_ops_api::CMD_FLAGS_BARRIER_DISABLE, wrNodes.data(), wrNodes.size(),
          device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // hang kernel expected status
  device_ops_api::dev_ops_api_kernel_launch_response_e hangKerExpectedRsp = sendAbortCmd_ ?
      device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED :
      device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_TIMEOUT_HANG;

  // Kernel launch
  stream.push_back(
      IDevOpsApiCmd::createCmd<KernelLaunchCmd>(
      device_ops_api::CMD_FLAGS_BARRIER_ENABLE,
      kernelEntryAddr, 0 /* No kernel args */,
      kernelExceptionSpace(deviceIdx), shireMask, 0, nullptr, 0,"",
      kertype == KernelTypes::EXCEP_KERNEL_TYPE ?
          device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_EXCEPTION : // exception kernel
          hangKerExpectedRsp)); // hang kernel

  info.tagId = stream.back();
  info.queueIdx = queueIdx;

  // kernel abort
  if (kertype == KernelTypes::HANG_KERNEL_TYPE && sendAbortCmd_)
    stream.push_back(
        IDevOpsApiCmd::createCmd<KernelAbortCmd>(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, info.tagId,
            device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS));

  // pull the exception buffer from device
  if (isReadExceptionContext_) {
      std::vector<device_ops_api::dma_read_node> contextNodes;
      // DMA list node
    auto readPtr = static_cast<void*>(getMmapBuffer(deviceIdx, kExceptionSize, false));
    contextNodes.push_back(
        fillDMAReadNode(templ::bit_cast<uint64_t>(readPtr),
            kernelExceptionSpace(deviceIdx), kExceptionSize));

      // create DMA read list command
      stream.push_back(
          IDevOpsApiCmd::createCmd<DmaReadListCmd>(
                  device_ops_api::CMD_FLAGS_BARRIER_ENABLE,
                  contextNodes.data(), contextNodes.size(),
                  device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

      info.data = static_cast<std::byte*> (readPtr);
      contexts.push_back(info);
    }
}

void
TestDevOpsApiKernelCmds::emptyKernelReadBackResults(int deviceIdx,
    std::vector<uint64_t> dummyResultAddrs,
    std::vector<CmdTag> &stream) {

  std::vector<device_ops_api::dma_read_node> rdNodes;
  for (size_t i = 0 ; i < dummyResultAddrs.size(); ++i) {
    rdNodes.push_back(fillDMAReadNode(templ::bit_cast<uint64_t>(getMmapBuffer(deviceIdx, kEmptyKerDummyRead, false)),
        dummyResultAddrs[i], kEmptyKerDummyRead));

    if (rdNodes.size() == 4 || i == dummyResultAddrs.size() - 1) {
      // create DMA read list command
      stream.push_back(
          IDevOpsApiCmd::createCmd<DmaReadListCmd>(
              i < 4 ? // Barrier only for first read to make sure that all kernels execution done
                  device_ops_api::CMD_FLAGS_BARRIER_ENABLE : device_ops_api::CMD_FLAGS_BARRIER_DISABLE,
                  rdNodes.data(), static_cast<uint32_t>(rdNodes.size()),
                  device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      rdNodes.clear();
    }
  }
}

uint64_t
TestDevOpsApiKernelCmds::handleEmptyKernelDMAListCmd(
    int deviceIdx, uint64_t shireMask,
    std::vector<CmdTag> &stream) {

  std::vector<device_ops_api::dma_write_node> wrNodes(1);

  // load elf
  auto kernelEntryAddr = loadElf(deviceIdx, KernelTypes::EMPTY_KERNEL_TYPE, wrNodes[0]);

  // create DMA write list command
  stream.push_back(
      IDevOpsApiCmd::createCmd<DmaWriteListCmd>(
          device_ops_api::CMD_FLAGS_BARRIER_DISABLE, wrNodes.data(), wrNodes.size(),
          device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  // dummy argument
  std::array<uint8_t, 8> dummyKernelArgs = {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF};
  // alocate space for dummy arguments
  auto kernelArgsAddr = getDmaWriteAddr(deviceIdx, ALIGN(sizeof(dummyKernelArgs), kCacheLineSize));

  // Add kernel launch command
  stream.push_back(
      IDevOpsApiCmd::createCmd<KernelLaunchCmd>(
          (device_ops_api::CMD_FLAGS_BARRIER_ENABLE | (flushL3_ ? device_ops_api::CMD_FLAGS_KERNEL_LAUNCH_FLUSH_L3 : 0)),
          kernelEntryAddr, kernelArgsAddr,
          kernelExceptionSpace(deviceIdx), shireMask, 0,
          templ::bit_cast<uint64_t*>(&dummyKernelArgs), sizeof(dummyKernelArgs), "",
          device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));

  // allocate 4KB space for dummy kernel result
  return getDmaWriteAddr(deviceIdx, ALIGN(kEmptyKerDummyRead, kCacheLineSize));
}

void TestDevOpsApiKernelCmds::validateUberKernelResult(std::vector<uint64_t*> layersResults) const{

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // Verification data
  const std::vector<uint64_t> refdata0(kUberKerLayerBuf0 / sizeof(uint64_t), 0xBEEFBEEFBEEFBEEFULL);
  const std::vector<uint64_t> refdata1(kUberKerLayerBuf1 / sizeof(uint64_t), 0xBEEFBEEFBEEFBEEFULL);

  for (size_t i = 0; i < layersResults.size(); i += 2) {
    ASSERT_EQ(memcmp(layersResults[i], refdata0.data(), kUberKerLayerBuf0), 0);
    ASSERT_EQ(memcmp(layersResults[i + 1], refdata1.data(), kUberKerLayerBuf1), 0);
  }

  TEST_VLOG(1) << "====> UBERKERNEL KERNEL RESPONSE DATA VERIFIED <====" << std::endl;
}

void TestDevOpsApiKernelCmds::handleUberKernelDMAListCmd(int deviceIdx,
    uint64_t shireMask, std::vector<uint64_t*> &layersResult,
    std::vector<CmdTag> &stream) {

  std::vector<device_ops_api::dma_write_node> wrNodes(1);
  std::vector<device_ops_api::dma_read_node> rdNodes;

  std::vector<std::vector<uint64_t>> vResultStorageUberKer;
  std::vector<uint64_t> vResultUberKer;

  std::array<UberLayerParameters_t, 2> uberKerArgs;

  // get device address
  auto devAddrLayer0 = getDmaWriteAddr(deviceIdx, kUberKerLayerBuf0);
  auto devAddrLayer1 = getDmaWriteAddr(deviceIdx, kUberKerLayerBuf1);
  auto uberKerArgSpace =  getDmaWriteAddr(deviceIdx, ALIGN(sizeof(uberKerArgs[0]) * uberKerArgs.size(), kCacheLineSize));

  uberKerArgs[0].data_ptr = devAddrLayer0;
  uberKerArgs[0].length = kUberKerLayerBuf0;
  uberKerArgs[0].shire_count = static_cast<uint32_t>(std::bitset<64>(shireMask).count());
  uberKerArgs[1].data_ptr = devAddrLayer1;
  uberKerArgs[1].length = kUberKerLayerBuf1;
  uberKerArgs[1].shire_count = static_cast<uint32_t>(std::bitset<64>(shireMask).count());

  // Load elf
  auto uberKerEntryAddr = loadElf(deviceIdx, KernelTypes::UBER_KERNEL_TYPE, wrNodes[0]);

  // create DMA write list command
  stream.push_back(
      IDevOpsApiCmd::createCmd<DmaWriteListCmd>(
          device_ops_api::CMD_FLAGS_BARRIER_DISABLE, wrNodes.data(), wrNodes.size(),
          device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

    // Kernel launch Command
  stream.push_back(IDevOpsApiCmd::createCmd<KernelLaunchCmd>(
      device_ops_api::CMD_FLAGS_BARRIER_ENABLE, uberKerEntryAddr, uberKerArgSpace, kernelExceptionSpace(deviceIdx),
      shireMask, 0, templ::bit_cast<uint64_t*>(uberKerArgs.data()), sizeof(uberKerArgs[0]) * uberKerArgs.size(), "",
      device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));

  // Read back data written by layer 0
  layersResult.push_back(static_cast<uint64_t*>(static_cast<void*>(getMmapBuffer(deviceIdx, kUberKerLayerBuf0, false))));
  rdNodes.push_back(fillDMAReadNode(templ::bit_cast<uint64_t>(layersResult.back()), devAddrLayer0, kUberKerLayerBuf0));


  // Read back data written by layer 1
  layersResult.push_back(static_cast<uint64_t*>(static_cast<void*>(getMmapBuffer(deviceIdx, kUberKerLayerBuf1, false))));
  rdNodes.push_back(fillDMAReadNode(templ::bit_cast<uint64_t>(layersResult.back()), devAddrLayer1, kUberKerLayerBuf1));

  // create DMA read list command
  stream.push_back(
      IDevOpsApiCmd::createCmd<DmaReadListCmd>(
          device_ops_api::CMD_FLAGS_BARRIER_ENABLE,
          rdNodes.data(), rdNodes.size(),
          device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
}

void TestDevOpsApiKernelCmds::cleanDMABuffer() {
  for (auto m : dmaBufferRefernces_)
    freeDmaBuffer(m);

  dmaBufferRefernces_.clear();
  allocatedWriteDMABuffers_.clear();
  allocatedReadDMABuffers_.clear();
}

uint8_t* TestDevOpsApiKernelCmds::getMmapBuffer(int devicesIdx, size_t bytes, bool write) {
  if (write) {
    auto p = allocatedWriteDMABuffers_[devicesIdx];
    allocatedWriteDMABuffers_[devicesIdx] += bytes;
    return p;
  }

  auto p = allocatedReadDMABuffers_[devicesIdx];
  allocatedReadDMABuffers_[devicesIdx] += bytes;
  return p;
}

void TestDevOpsApiKernelCmds::calculateDMABuffer(const kernelContainer_t kernels, int totalDevices) {

  std::vector<int> devicesWriteMemSize(totalDevices);
  std::vector<int> devicesReadMemSize(totalDevices);

  for (const auto& perQueueKernels : kernels) {
    size_t perDevWriteMemBytes = 0;
    size_t perDevReadMemBytes = 0;
    auto deviceIdx = perQueueKernels.deviceIdx;
    for (auto kerType : perQueueKernels.kernels) {
      switch(kerType) {
        case KernelTypes::ADD_KERNEL_TYPE: {
          // Two add vector and their results
          perDevWriteMemBytes += ALIGN((addKerNumElems_ * sizeof(int)), kCacheLineSize) * 2;
          perDevReadMemBytes += ALIGN((addKerNumElems_ * sizeof(int)), kCacheLineSize);
          break;
        }
        case KernelTypes::EXCEP_KERNEL_TYPE:
        case KernelTypes::HANG_KERNEL_TYPE: {
          // Read exception
          perDevReadMemBytes += kExceptionSize;
          break;
        }
        case KernelTypes::EMPTY_KERNEL_TYPE: {
          perDevReadMemBytes += kEmptyKerDummyRead;
          break;
        }
        case KernelTypes::UBER_KERNEL_TYPE: {
          perDevWriteMemBytes += kUberKerLayerBuf0 + kUberKerLayerBuf1;
          perDevReadMemBytes += kUberKerLayerBuf0 + kUberKerLayerBuf1;
          break;
        }
        default:
          break;
      }

      // 2 MB for kernel elf
      perDevWriteMemBytes += 1024 * 1024 * 2;

      // 1 MB margin
      perDevReadMemBytes += 1024 * 1024 * 1;
    }

    devicesWriteMemSize[deviceIdx] += perDevWriteMemBytes;
    devicesReadMemSize[deviceIdx] += perDevReadMemBytes;
  }

  // alocate DMA memory per device
  for (int dev = 0; dev < totalDevices ; ++dev) {
    // Write DMA memory
    dmaBufferRefernces_.push_back(allocDmaBuffer(dev, devicesWriteMemSize[dev], true));
    allocatedWriteDMABuffers_[dev] = static_cast<uint8_t*>(dmaBufferRefernces_.back());

    // Read DMA memory
    dmaBufferRefernces_.push_back(allocDmaBuffer(dev, devicesReadMemSize[dev], false));
    allocatedReadDMABuffers_[dev] = static_cast<uint8_t*>(dmaBufferRefernces_.back());
  }
}

kernelContainer_t
TestDevOpsApiKernelCmds::buildKernelsInfo(std::vector<KernelTypes> totalKer, bool singleDevice, bool singleQueue, bool calculateDmaListSize) {
  int kernelsCount = 0;
  kernelContainer_t kernels;
  auto perQueueKernels = totalKer;

  auto devCount = singleDevice ? 1 : getDevicesCount();
  for (auto devIdx = 0; devIdx < devCount; ++devIdx) {
    auto queueCount = singleQueue ? 1 : getSqCount(devIdx);
    for (auto queueIdx = 0; queueIdx < queueCount; ++queueIdx) {
      if (totalKer.size() > 1) {
        // Use all remaining kernels at the end
        int perQueueKersCount =
            (devIdx == (devCount - 1) && queueIdx == (queueCount - 1)) ?
                static_cast<int>(totalKer.size()) - kernelsCount :
                static_cast<int>(totalKer.size()) / devCount / queueCount;
        perQueueKernels = {totalKer.begin() + kernelsCount, totalKer.begin() + kernelsCount + perQueueKersCount};
        kernelsCount += perQueueKersCount;
      }
      // save kernel info
      kernels.push_back({ devIdx, queueIdx, perQueueKernels });
    }
  }

  if (calculateDmaListSize)
	  calculateDMABuffer(kernels, devCount);

  return kernels;
}

void TestDevOpsApiKernelCmds::launchKernelDMAListCmds(uint64_t shireMask, std::vector<KernelTypes> totalKer, bool singleDevice, bool singleQueue) {
  // Add kernel parameters
  std::vector<AddKerInfo> addKernelSumVectorAB;
  std::vector<int*> addKernelResultVectorAB;

  // Exception/hang kernel parameters
  std::vector<ExcepContextInfo> exceptionContexts;

  // Empty kernel parameters
  std::vector<uint64_t> emptyKerDummyAddrs;

  // Uber kernel parametsr
  std::vector<uint64_t*> uberKerlayersResult;

  kernelContainer_t kernels = buildKernelsInfo(totalKer, singleDevice, singleQueue);

  for (const auto& perQueueKernels : kernels) {
    auto devIdx = perQueueKernels.deviceIdx;
    auto queueIdx = perQueueKernels.queueIdx;
    std::vector<CmdTag> stream;
    std::vector<uint64_t> addKernelResultDevAddrs;
    for (auto kerType : perQueueKernels.kernels) {
      switch(kerType) {
        case KernelTypes::ADD_KERNEL_TYPE: {

          std::vector<int> sumOfvectors;
          addKernelResultDevAddrs.push_back(handleAddKernelDMAListCmd(devIdx, shireMask, sumOfvectors, stream));
          addKernelSumVectorAB.push_back({devIdx, queueIdx, sumOfvectors});
          break;
        }
          case KernelTypes::EXCEP_KERNEL_TYPE:
          case KernelTypes::HANG_KERNEL_TYPE: {

            handleExceptionOrAbortKernelDMAListCmd(devIdx, queueIdx, shireMask,kerType, exceptionContexts, stream);
            break;
          }
          case KernelTypes::EMPTY_KERNEL_TYPE: {

            emptyKerDummyAddrs.push_back(handleEmptyKernelDMAListCmd(devIdx, shireMask, stream));
            break;
          }
          case KernelTypes::UBER_KERNEL_TYPE: {

            handleUberKernelDMAListCmd(devIdx, shireMask, uberKerlayersResult, stream);
            break;
          }
          default:
            break;
      }
    }

    // Read back add kernel results
    addKernelResultReadBackPerQueue(devIdx, addKernelResultVectorAB, addKernelResultDevAddrs, stream);

    // Read back empty kernel result
    emptyKernelReadBackResults(devIdx, emptyKerDummyAddrs, stream);

    // Save stream against deviceIdx and queueIdx
    insertStream(devIdx, queueIdx, std::move(stream));
  }

  // Execute all commands
  executeAsync();

  // validate Add kernel commands
  validataAddKernel(addKernelSumVectorAB, addKernelResultVectorAB);

 // print exception context
  printExceptionContext(exceptionContexts);

  // validate uber kernel command
  validateUberKernelResult(uberKerlayersResult);

  deleteStreams();
  cleanDMABuffer();
}

/* Kernel Functional Tests */
void TestDevOpsApiKernelCmds::launchAddVectorKernelListCmd(uint64_t shire_mask, KernelTypes kernelType) {
  addKernelType_ = kernelType;
  addKerNumElems_ = 10496;
  launchKernelDMAListCmds(shire_mask, generateKernelTypes(KernelTypes::ADD_KERNEL_TYPE));
}
void TestDevOpsApiKernelCmds::launchUberKernelListCmd(uint64_t shire_mask) {
  launchKernelDMAListCmds(shire_mask, generateKernelTypes(KernelTypes::UBER_KERNEL_TYPE));
}

void TestDevOpsApiKernelCmds::launchEmptyKernelListCmd(uint64_t shire_mask) {
  launchKernelDMAListCmds(shire_mask, generateKernelTypes(KernelTypes::EMPTY_KERNEL_TYPE));
}

void TestDevOpsApiKernelCmds::launchExceptionKernelListCmd(uint64_t shire_mask) {
  launchKernelDMAListCmds(shire_mask, generateKernelTypes(KernelTypes::EXCEP_KERNEL_TYPE));
}

void TestDevOpsApiKernelCmds::launchHangKernelListCmd(uint64_t shire_mask, bool sendAbortCmd){
  sendAbortCmd_ = sendAbortCmd;
  launchKernelDMAListCmds(shire_mask, generateKernelTypes(KernelTypes::HANG_KERNEL_TYPE));
}

/* Kernel Stress Tests*/
void TestDevOpsApiKernelCmds::backToBackSameKernelLaunchListCmds(bool singleDevice, bool singleQueue, uint64_t totalKer, uint64_t shire_mask) {
  launchKernelDMAListCmds(shire_mask, generateKernelTypes(KernelTypes::ADD_KERNEL_TYPE, totalKer), singleDevice, singleQueue);
}

void TestDevOpsApiKernelCmds::backToBackDifferentKernelLaunchListCmds(bool singleDevice, bool singleQueue, uint64_t totalKer, uint64_t shire_mask) {
  isReadExceptionContext_ = false;
  launchKernelDMAListCmds(shire_mask, generateKernelTypes(KernelTypes::ADD_KERNEL_TYPE, totalKer, 0x3), singleDevice, singleQueue);
}

void TestDevOpsApiKernelCmds::backToBackEmptyKernelLaunchListCmds(uint64_t totalKer, uint64_t shire_mask, bool flushL3) {
  flushL3_ = flushL3;
  launchKernelDMAListCmds(shire_mask, generateKernelTypes(KernelTypes::EMPTY_KERNEL_TYPE, totalKer));
}
