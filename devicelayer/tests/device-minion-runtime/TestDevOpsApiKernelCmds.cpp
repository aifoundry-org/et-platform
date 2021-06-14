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
/**********************************************************
 *                                                         *
 *               Kernel Functional Tests                   *
 *                                                         *
 **********************************************************/
void TestDevOpsApiKernelCmds::launchAddVectorKernel_PositiveTesting_4_1(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> streamAddKer;

  std::vector<std::vector<ELFIO::elfio>> readersStorageAddKer;
  std::vector<ELFIO::elfio> readersAddKer;
  auto elfPathAddKer = (fs::path(FLAGS_kernels_dir) / fs::path("add_vector.elf")).string();

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
      auto vAHostVirtAddr = reinterpret_cast<uint64_t>(vDataAStorageAddKer.back().data());
      auto vAHostPhysAddr = vAHostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      streamAddKer.push_back(IDevOpsApiCmd::createDataWriteCmd(false, vADevAddr, vAHostVirtAddr, vAHostPhysAddr,
                                                               vDataAStorageAddKer.back().size() * sizeof(int),
                                                               device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      auto vBHostVirtAddr = reinterpret_cast<uint64_t>(vDataBStorageAddKer.back().data());
      auto vBHostPhysAddr = vBHostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      streamAddKer.push_back(IDevOpsApiCmd::createDataWriteCmd(false, vBDevAddr, vBHostVirtAddr, vBHostPhysAddr,
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
      streamAddKer.push_back(
        IDevOpsApiCmd::createKernelLaunchCmd(true, kernelEntryDevAddrAddKer, kernelArgsDevAddr, kernelExceptionDevAddr,
                                             shire_mask, 0, reinterpret_cast<void*>(&parameters), sizeof(parameters),
                                             device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));

      // Read back Kernel Results from device
      vTempDataResultAddKer.resize(numElemsAddKer, 0);
      vResultStorageAddKer.push_back(std::move(vTempDataResultAddKer));
      auto vResultHostVirtAddr = reinterpret_cast<uint64_t>(vResultStorageAddKer.back().data());
      auto vResultHostPhysAddr =
        vResultHostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      streamAddKer.push_back(
        IDevOpsApiCmd::createDataReadCmd(true, vResultDevAddr, vResultHostVirtAddr, vResultHostPhysAddr,
                                         vResultStorageAddKer.back().size() * sizeof(vTempDataResultAddKer[0]),
                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

      // Save off the data
      streams_.try_emplace(key(deviceIdxAddKer, queueIdxAddKer), std::move(streamAddKer));
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

  TEST_VLOG(1) << "====> ADD TWO VECTORS KERNEL RESPONSE DATA VERIFIED <====" << std::endl;
}

void TestDevOpsApiKernelCmds::launchUberKernel_PositiveTesting_4_4(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> streamUberKer;

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
  layer_parameters_t launchArgs[2];

  int deviceCountUberKer = getDevicesCount();
  for (int deviceIdxUberKer = 0; deviceIdxUberKer < deviceCountUberKer; deviceIdxUberKer++) {
    int queueCountUberKer = getSqCount(deviceIdxUberKer);
    readersUberKer.resize(queueCountUberKer);
    for (uint8_t queueIdxUberKer = 0; queueIdxUberKer < queueCountUberKer; queueIdxUberKer++) {
      auto devAddrBufLayer0 = getDmaWriteAddr(deviceIdxUberKer, bufSizeLayer0);
      auto devAddrBufLayer1 = getDmaWriteAddr(deviceIdxUberKer, bufSizeLayer1);
      TEST_VLOG(1) << "SQ : " << queueIdxUberKer << " devAddrBufLayer0: 0x" << std::hex << devAddrBufLayer0;
      TEST_VLOG(1) << "SQ : " << queueIdxUberKer << " devAddrBufLayer1: 0x" << std::hex << devAddrBufLayer1;
      auto devAddrKernelArgs = getDmaWriteAddr(deviceIdxUberKer, ALIGN(sizeof(launchArgs), kCacheLineSize));
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
      streamUberKer.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
        device_ops_api::CMD_FLAGS_BARRIER_ENABLE, kernelEntryDevAddrUberKer, devAddrKernelArgs, kernelExceptionDevAddr,
        shire_mask, 0, reinterpret_cast<void*>(launchArgs), sizeof(launchArgs),
        device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));

      // Read back data written by layer 0
      vResultUberKer.resize(numElemsLayer0, 0xEEEEEEEEEEEEEEEEULL);
      vResultStorageUberKer.push_back(std::move(vResultUberKer));
      auto hostVirtAddr = reinterpret_cast<uint64_t>(vResultStorageUberKer.back().data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      streamUberKer.push_back(
        IDevOpsApiCmd::createDataReadCmd(device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrBufLayer0, hostVirtAddr,
                                         hostPhysAddr, vResultStorageUberKer.back().size() * sizeof(vResultUberKer[0]),
                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

      // Read back data written by layer 1
      vResultUberKer.resize(numElemsLayer1, 0xEEEEEEEEEEEEEEEEULL);
      vResultStorageUberKer.push_back(std::move(vResultUberKer));
      hostVirtAddr = reinterpret_cast<uint64_t>(vResultStorageUberKer.back().data());
      hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      streamUberKer.push_back(
        IDevOpsApiCmd::createDataReadCmd(device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrBufLayer1, hostVirtAddr,
                                         hostPhysAddr, vResultStorageUberKer.back().size() * sizeof(vResultUberKer[0]),
                                         device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

      // Move stream of commands to streams_
      streams_.try_emplace(key(deviceIdxUberKer, queueIdxUberKer), std::move(streamUberKer));
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
  TEST_VLOG(1) << "====> UBERKERNEL KERNEL RESPONSE DATA VERIFIED <====" << std::endl;
}

void TestDevOpsApiKernelCmds::launchEmptyKernel_PositiveTesting_4_5(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> streamEmptyKer;
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
      auto devAddrKernelArgs = getDmaWriteAddr(deviceIdx,
        ALIGN(dummyKernelArgs.size() * sizeof(unsigned long int), kCacheLineSize));
      // allocate 4KB space for dummy kernel result
      auto devAddrKernelResult = getDmaWriteAddr(deviceIdx, ALIGN(sizeof(0x1000), kCacheLineSize));
      // allocate 1MB space for kernel error/exception buffer
      auto kernelExceptionDevAddr = getDmaWriteAddr(deviceIdx, 0x100000);
      // kernel launch
      streamEmptyKer.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
        device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kernelEntryDevAddrEmptyKer, devAddrKernelArgs,
        kernelExceptionDevAddr, shire_mask, 0, static_cast<void*>(dummyKernelArgs.data()),
        static_cast<int>(dummyKernelArgs.size() * sizeof(unsigned long int)),
        device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));
      // Do a dummy read back Kernel Results from device
      vResult.resize(0x1000, 0);
      vResultStorage.push_back(std::move(vResult));
      auto hostVirtAddr = reinterpret_cast<uint64_t>(vResultStorage.back().data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      streamEmptyKer.push_back(IDevOpsApiCmd::createDataReadCmd(
        device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrKernelResult, hostVirtAddr, hostPhysAddr,
        vResultStorage.back().size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

      // Move stream of commands to streams_
      streams_.try_emplace(key(deviceIdx, queueIdx), std::move(streamEmptyKer));
    }
    readersStorageEmptyKer.push_back(std::move(readers));
  }
  executeAsync();

  TEST_VLOG(1) << "====> EMPTY KERNEL DONE <====" << std::endl;
}

void TestDevOpsApiKernelCmds::launchExceptionKernel_NegativeTesting_4_6(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> streamExceptKer;
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
      auto kernelCmd = IDevOpsApiCmd::createKernelLaunchCmd(
        device_ops_api::CMD_FLAGS_BARRIER_ENABLE, kernelEntryDevAddrExceptKer, 0 /* No kernel args */,
        kernelExceptionDevAddr, shire_mask, 0, nullptr, 0, device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_EXCEPTION,
        "exception.elf");
      kernelLaunchTagId.push_back(kernelCmd->getCmdTagId());
      streamExceptKer.push_back(std::move(kernelCmd));

      // pull the exception buffer from device
      vResult.resize(0x1000, 0);
      vResultStorage.push_back(std::move(vResult));
      auto hostVirtAddr = reinterpret_cast<uint64_t>(vResultStorage.back().data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      streamExceptKer.push_back(IDevOpsApiCmd::createDataReadCmd(
        device_ops_api::CMD_FLAGS_BARRIER_ENABLE, kernelExceptionDevAddr, hostVirtAddr, hostPhysAddr,
        vResultStorage.back().size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

      // Move stream of commands to streams_
      streams_.try_emplace(key(deviceIdxExceptKer, queueIdxExceptKer), std::move(streamExceptKer));
    }
    readersStorageExceptKer.push_back(std::move(readersExceptKer));
  }
  executeAsync();

  // print the exception buffer for the first two shires
  for (int deviceIdxExceptKer = 0, i = 0; deviceIdxExceptKer < deviceCountExceptKer; deviceIdxExceptKer++) {
    int queueCountExceptKer = getSqCount(deviceIdxExceptKer);
    for (int queueIdxExceptKer = 0; queueIdxExceptKer < queueCountExceptKer; queueIdxExceptKer++) {
      printErrorContext(queueIdxExceptKer, reinterpret_cast<void*>(vResultStorage[i].data()), 0x3,
                        kernelLaunchTagId[i]);
      i++;
    }
  }

  TEST_VLOG(1) << "====> EXCEPTION KERNEL DONE <====" << std::endl;
}

void TestDevOpsApiKernelCmds::abortHangKernel_PositiveTesting_4_10(uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> streamHangKer;
  std::vector<std::vector<ELFIO::elfio>> readersStorageHangKer;
  std::vector<ELFIO::elfio> readersHangKer;
  auto elfPathHangKer = (fs::path(FLAGS_kernels_dir) / fs::path("hang.elf")).string();

  std::vector<std::vector<uint8_t>> vResultStorageHangKer;
  std::vector<uint8_t> vResultHangKer;
  std::vector<device_ops_api::tag_id_t> kernelLaunchTagId;
  int tagIdIdx = 0;
  int deviceCountHangKer = getDevicesCount();
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
      auto kernelCmd = IDevOpsApiCmd::createKernelLaunchCmd(
        device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kernelEntryDevAddrHangKer, 0 /* No kernel args */,
        devAddrkernelException, shire_mask, 0, nullptr, 0,
        device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED, "hang.elf");
      kernelLaunchTagId.push_back(kernelCmd->getCmdTagId());
      streamHangKer.push_back(std::move(kernelCmd));

      // Kernel Abort
      streamHangKer.push_back(
        IDevOpsApiCmd::createKernelAbortCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kernelLaunchTagId[tagIdIdx],
                                            device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS));
      tagIdIdx++;
      // pull the exception buffer from device
      vResultHangKer.resize(0x100000, 0);
      vResultStorageHangKer.push_back(std::move(vResultHangKer));
      auto hostVirtAddr = reinterpret_cast<uint64_t>(vResultStorageHangKer.back().data());
      auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
      streamHangKer.push_back(IDevOpsApiCmd::createDataReadCmd(
        device_ops_api::CMD_FLAGS_BARRIER_ENABLE, devAddrkernelException, hostVirtAddr, hostPhysAddr,
        vResultStorageHangKer.back().size(), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

      // Move stream of commands to streams_
      streams_.try_emplace(key(deviceIdxHangKer, queueIdxHangKer), std::move(streamHangKer));
    }
    readersStorageHangKer.push_back(std::move(readersHangKer));
  }
  executeAsync();

  // print the exception buffer for the first two shires
  for (int deviceIdxHangKer = 0, i = 0; deviceIdxHangKer < deviceCountHangKer; deviceIdxHangKer++) {
    int queueCountHangKer = getSqCount(deviceIdxHangKer);
    for (int queueIdxHangKer = 0; queueIdxHangKer < queueCountHangKer; queueIdxHangKer++) {
      printErrorContext(queueIdxHangKer, reinterpret_cast<void*>(vResultStorageHangKer[i].data()), 0x3,
                        kernelLaunchTagId[i]);
      i++;
    }
  }
  TEST_VLOG(1) << "====> HANG KERNEL DONE <====" << std::endl;
}

void TestDevOpsApiKernelCmds::kernelAbortCmd_InvalidTagIdNegativeTesting_6_2() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> streamAbortCmd;
  int deviceCountAbortCmd = getDevicesCount();
  for (int deviceIdxAbortCmd = 0; deviceIdxAbortCmd < deviceCountAbortCmd; deviceIdxAbortCmd++) {
    auto queueCountAbortCmd = getSqCount(deviceIdxAbortCmd);
    for (uint8_t queueIdxAbortCmd = 0; queueIdxAbortCmd < queueCountAbortCmd; queueIdxAbortCmd++) {
      streamAbortCmd.push_back(
        IDevOpsApiCmd::createKernelAbortCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, 0xbeef /* invalid tagId */,
                                            device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_INVALID_TAG_ID));

      // Move stream of commands to streams_[queueIdx]
      streams_.try_emplace(key(deviceIdxAbortCmd, queueIdxAbortCmd), std::move(streamAbortCmd));
    }
  }
  executeAsync();
}

/**********************************************************
 *                                                         *
 *                  Kernel Stress Tests                    *
 *                                                         *
 **********************************************************/
void TestDevOpsApiKernelCmds::backToBackSameKernelLaunchCmds_3_1(bool singleDevice, bool singleQueue, uint64_t totalKer,
                                                                 uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> streamSameKer;
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
  // create device params structure
  struct Params {
    uint64_t vASameKer;
    uint64_t vBSameKer;
    uint64_t vResultSameKer;
    int numElementsSameKer;
  };
  auto deviceCountSameKer = singleDevice ? 1 : getDevicesCount();
  for (int deviceIdxSameKer = 0; deviceIdxSameKer < deviceCountSameKer; deviceIdxSameKer++) {
    auto queueCountSameKer = singleQueue ? 1 : getSqCount(deviceIdxSameKer);
    readersSameKer.resize(totalKer);
    int kernelCount = 0;
    for (uint16_t queueIdxSameKer = 0; queueIdxSameKer < queueCountSameKer; queueIdxSameKer++) {
      std::vector<uint64_t> devAddrVecResult(totalKer / queueCountSameKer);
      for (int i = 0; i < totalKer / queueCountSameKer; i++) {
        generateRandomData(numElemsSameKer, vDataAStorageSameKer, vDataBStorageSameKer, vSumStorageSameKer);

        auto hostVirtAddrVecA = reinterpret_cast<uint64_t>(vDataAStorageSameKer.back().data());
        auto hostPhysAddrVecA = hostVirtAddrVecA; // Should be handled in SysEmu, userspace should not fill this value
        auto hostVirtAddrVecB = reinterpret_cast<uint64_t>(vDataBStorageSameKer.back().data());
        auto hostPhysAddrVecB = hostVirtAddrVecB; // Should be handled in SysEmu, userspace should not fill this value
        // Load kernel ELF
        uint64_t kernelEntryDevAddrSameKer;
        loadElfToDevice(deviceIdxSameKer, readersSameKer[kernelCount], elfPathSameKer, streamSameKer,
                        kernelEntryDevAddrSameKer);
        // Copy kernel input data to device
        auto dataLoadAddr = getDmaWriteAddr(deviceIdxSameKer, 2 * alignedBufSize);
        auto devAddrVecA = dataLoadAddr;
        auto devAddrVecB = devAddrVecA + alignedBufSize;
        streamSameKer.push_back(IDevOpsApiCmd::createDataWriteCmd(
          device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devAddrVecA, hostVirtAddrVecA, hostPhysAddrVecA,
          vDataAStorageSameKer.back().size() * sizeof(int), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
        streamSameKer.push_back(IDevOpsApiCmd::createDataWriteCmd(
          device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devAddrVecB, hostVirtAddrVecB, hostPhysAddrVecB,
          vDataBStorageSameKer.back().size() * sizeof(int), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

        // allocate space for kernel args
        devAddrVecResult[i] = getDmaWriteAddr(deviceIdxSameKer, alignedBufSize);
        auto devAddrKernelArgs = getDmaWriteAddr(deviceIdxSameKer, ALIGN(sizeof(Params), kCacheLineSize));
        // allocate 1MB space for kernel error/exception buffer
        auto devAddrkernelException = getDmaWriteAddr(deviceIdxSameKer, 0x100000);

        Params kerParams = {devAddrVecA, devAddrVecB, devAddrVecResult[i], numElemsSameKer};
        // Launch Kernel Command
        streamSameKer.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
          device_ops_api::CMD_FLAGS_BARRIER_ENABLE, kernelEntryDevAddrSameKer, devAddrKernelArgs,
          devAddrkernelException, shire_mask, 0, reinterpret_cast<void*>(&kerParams), sizeof(kerParams),
          device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));
        kernelCount++;
      }

      // Read back Kernel Results from device
      for (int i = 0; i < totalKer / queueCountSameKer; i++) {
        vTempDataResultSameKer.resize(numElemsSameKer, 0);
        vResultStorageSameKer.push_back(std::move(vTempDataResultSameKer));
        auto hostVirtAddrRes = reinterpret_cast<uint64_t>(vResultStorageSameKer.back().data());
        auto hostPhysAddrRes = hostVirtAddrRes; // Should be handled in SysEmu, userspace should not fill this value
        streamSameKer.push_back(IDevOpsApiCmd::createDataReadCmd(
          (i == 0) ? device_ops_api::CMD_FLAGS_BARRIER_ENABLE
                   : device_ops_api::CMD_FLAGS_BARRIER_DISABLE, /* Barrier only for first read to make sure that all
                                                                   kernels execution done */
          devAddrVecResult[i], hostVirtAddrRes, hostPhysAddrRes,
          vResultStorageSameKer.back().size() * sizeof(vTempDataResultSameKer[0]),
          device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      }
      // Move stream of commands to streams_
      streams_.try_emplace(key(deviceIdxSameKer, queueIdxSameKer), std::move(streamSameKer));
    }
    readersStorageSameKer.push_back(std::move(readersSameKer));
  }

  executeAsync();

  // Skip data validation in case of loopback driver
  if (FLAGS_loopback_driver) {
    return;
  }

  // Verify Vector's Data
  for (int deviceIdxSameKer = 0, i = 0; deviceIdxSameKer < deviceCountSameKer; deviceIdxSameKer++) {
    int queueCountSameKer = singleQueue ? 1 : getSqCount(deviceIdxSameKer);
    for (int queueIdxSameKer = 0; queueIdxSameKer < queueCountSameKer; queueIdxSameKer++) {
      for (int kernelIdx = 0; kernelIdx < totalKer / queueCountSameKer; kernelIdx++) {
        ASSERT_EQ(vSumStorageSameKer[i], vResultStorageSameKer[i])
          << "Vectors result mismatch for device: " << deviceIdxSameKer << ", queue: " << queueIdxSameKer
          << " at Index: " << i;
        i++;
      }
    }
  }

  TEST_VLOG(1) << "====> BACK TO BACK " << totalKer << " KERNEL LAUNCH (ADD VECTORS KERNEL) DATA VERIFIED <====\n"
               << std::endl;
}

void TestDevOpsApiKernelCmds::backToBackDifferentKernelLaunchCmds_3_2(bool singleDevice, bool singleQueue,
                                                                      uint64_t totalKer, uint64_t shire_mask) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> streamDiffKer;
  int numElemsDiffKer = 1024;
  enum kernelTypes { addKerType = 0, excKerType, hangKerType };
  std::vector<uint8_t> kerTypes(totalKer);
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
  for (int deviceIdxDiffKer = 0; deviceIdxDiffKer < deviceCountDiffKer; deviceIdxDiffKer++) {
    auto queueCountDiffKer = singleQueue ? 1 : getSqCount(deviceIdxDiffKer);
    readersDiffKer.resize(totalKer);
    for (uint16_t queueIdxDiffKer = 0; queueIdxDiffKer < queueCountDiffKer; queueIdxDiffKer++) {
      std::vector<uint64_t> devAddrVecResult(totalAddKer);
      int addKernelPerSq = 0;
      for (int i = 0; i < totalKer / queueCountDiffKer; i++) {
        device_ops_api::tag_id_t kernelLaunchTagId;

        switch (kerTypes[kerCount]) {
        case addKerType: /* Add Kernel */ {
          generateRandomData(numElemsDiffKer, vDataAStorageDiffKer, vDataBStorageDiffKer, vSumStorageDiffKer);
          uint64_t addKernelEntryAddrDiffKer;
          loadElfToDevice(deviceIdxDiffKer, readersDiffKer[kerCount], addKerElfPath, streamDiffKer,
                          addKernelEntryAddrDiffKer);
          // Allocate device space for vector A, B and resultant vectors (totalAddKer)
          auto dataLoadAddr = getDmaWriteAddr(deviceIdxDiffKer, 2 * alignedBufSize);
          auto devAddrVecA = dataLoadAddr;
          auto devAddrVecB = devAddrVecA + alignedBufSize;
          // Copy kernel input data to device
          auto hostVirtAddrA = reinterpret_cast<uint64_t>(vDataAStorageDiffKer.back().data());
          auto hostPhysAddrA = hostVirtAddrA; // Should be handled in SysEmu, userspace should not fill this value
          streamDiffKer.push_back(IDevOpsApiCmd::createDataWriteCmd(
            device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devAddrVecA, hostVirtAddrA, hostPhysAddrA,
            vDataAStorageDiffKer.back().size() * sizeof(int), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
          auto hostVirtAddrB = reinterpret_cast<uint64_t>(vDataBStorageDiffKer.back().data());
          auto hostPhysAddrB = hostVirtAddrB; // Should be handled in SysEmu, userspace should not fill this value
          streamDiffKer.push_back(IDevOpsApiCmd::createDataWriteCmd(
            device_ops_api::CMD_FLAGS_BARRIER_DISABLE, devAddrVecB, hostVirtAddrB, hostPhysAddrB,
            vDataBStorageDiffKer.back().size() * sizeof(int), device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

          // Copy kernel args to device
          devAddrVecResult[addKernelPerSq] = getDmaWriteAddr(deviceIdxDiffKer, alignedBufSize);
          addKerParams = {devAddrVecA, devAddrVecB, devAddrVecResult[addKernelPerSq], numElemsDiffKer};
          auto devAddrKernelArgs = getDmaWriteAddr(deviceIdxDiffKer, ALIGN(sizeof(Params), kCacheLineSize));
          // allocate 1MB space for kernel error/exception buffer
          auto devAddrkernelException = getDmaWriteAddr(deviceIdxDiffKer, 0x100000);

          // Launch Kernel Command for add kernel
          streamDiffKer.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
            device_ops_api::CMD_FLAGS_BARRIER_ENABLE, addKernelEntryAddrDiffKer, devAddrKernelArgs,
            devAddrkernelException, shire_mask, 0, reinterpret_cast<void*>(&addKerParams), sizeof(Params),
            device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));
          kerCount++;
          addKernelPerSq++;
          break;
        }
        case excKerType: {
          uint64_t excepKerEntryAddr;
          loadElfToDevice(deviceIdxDiffKer, readersDiffKer[kerCount], excepKerElfPath, streamDiffKer,
                          excepKerEntryAddr);
          // allocate 1MB space for kernel error/exception buffer
          auto devAddrkernelException = getDmaWriteAddr(deviceIdxDiffKer, 0x100000);
          // Launch Kernel Command for exception kernel
          streamDiffKer.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
            device_ops_api::CMD_FLAGS_BARRIER_ENABLE, excepKerEntryAddr, 0 /* No kernel args */, devAddrkernelException,
            shire_mask, 0, reinterpret_cast<void*>(&addKerParams), sizeof(Params),
            device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_EXCEPTION));
          kerCount++;
          break;
        }
        case hangKerType: {
          uint64_t hangKerEntryAddr;
          loadElfToDevice(deviceIdxDiffKer, readersDiffKer[kerCount], hangKerElfPath, streamDiffKer, hangKerEntryAddr);
          // allocate 1MB space for kernel error/exception buffer
          auto devAddrkernelException = getDmaWriteAddr(deviceIdxDiffKer, 0x100000);
          // Kernel launch
          auto kernelCmd = IDevOpsApiCmd::createKernelLaunchCmd(
            device_ops_api::CMD_FLAGS_BARRIER_ENABLE, hangKerEntryAddr, 0 /* No kernel args */, devAddrkernelException,
            shire_mask, 0, nullptr, 0, device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED);
          kernelLaunchTagId = kernelCmd->getCmdTagId();
          streamDiffKer.push_back(std::move(kernelCmd));
          // Kernel Abort
          streamDiffKer.push_back(
            IDevOpsApiCmd::createKernelAbortCmd(device_ops_api::CMD_FLAGS_BARRIER_DISABLE, kernelLaunchTagId,
                                                device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS));
          kerCount++;
          break;
        }
        default:
          break;
        }
      }
      kerCountStorage.try_emplace(key(deviceIdxDiffKer, queueIdxDiffKer), addKernelPerSq);
      // Read back add Kernel Results from device
      for (int i = 0; i < addKernelPerSq; i++) {
        vTempDataResultDiffKer.resize(numElemsDiffKer, 0);
        vResultStorageDiffKer.push_back(std::move(vTempDataResultDiffKer));
        auto hostVirtAddrRes = reinterpret_cast<uint64_t>(vResultStorageDiffKer.back().data());
        auto hostPhysAddrRes = hostVirtAddrRes; // Should be handled in SysEmu, userspace should not fill this value
        streamDiffKer.push_back(IDevOpsApiCmd::createDataReadCmd(
          (i == 0) ? device_ops_api::CMD_FLAGS_BARRIER_ENABLE
                   : device_ops_api::CMD_FLAGS_BARRIER_DISABLE, /* Barrier only for first read to make sure that all
                                                                   kernels execution done */
          devAddrVecResult[i], hostVirtAddrRes, hostPhysAddrRes,
          vResultStorageDiffKer.back().size() * sizeof(vTempDataResultDiffKer[0]),
          device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      }
      // Move stream of commands to streams_
      streams_.try_emplace(key(deviceIdxDiffKer, queueIdxDiffKer), std::move(streamDiffKer));
    }
    readersStorageDiffKer.push_back(std::move(readersDiffKer));
  }
  executeAsync();

  if (FLAGS_loopback_driver) {
    return;
  }

  // Verify Vector's Data
  for (int deviceIdxDiffKer = 0, i = 0; deviceIdxDiffKer < deviceCountDiffKer; deviceIdxDiffKer++) {
    int queueCountDiffKer = singleQueue ? 1 : getSqCount(deviceIdxDiffKer);
    for (int queueIdxDiffKer = 0; queueIdxDiffKer < queueCountDiffKer; queueIdxDiffKer++) {
      for (int kernelIdx = 0; kernelIdx < kerCountStorage[key(deviceIdxDiffKer, queueIdxDiffKer)]; kernelIdx++) {
        ASSERT_EQ(vSumStorageDiffKer[i], vResultStorageDiffKer[i])
          << "Vectors result mismatch for device: " << deviceIdxDiffKer << ", queue: " << queueIdxDiffKer
          << " at Index: " << i;
        i++;
      }
    }
  }

  TEST_VLOG(1) << "====> BACK TO BACK " << totalKer << " DIFFERENT KERNEL LAUNCH VERIFIED" << std::endl;
}

void TestDevOpsApiKernelCmds::backToBackEmptyKernelLaunch_3_3(uint64_t totalKer, uint64_t shire_mask, bool flushL3) {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> streamEmptyKer;
  std::vector<std::vector<ELFIO::elfio>> readersStorageEmptyKer;
  std::vector<ELFIO::elfio> readersEmptyKer;
  auto elfPathEmptyKer = (fs::path(FLAGS_kernels_dir) / fs::path("empty.elf")).string();

  std::vector<std::vector<uint8_t>> vResultStorageEmptyKer;
  std::vector<uint8_t> vResultEmptyKer;
  uint8_t dummyKernelArgs[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF};
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
        streamEmptyKer.push_back(IDevOpsApiCmd::createKernelLaunchCmd(
          (device_ops_api::CMD_FLAGS_BARRIER_ENABLE | (flushL3 ? device_ops_api::CMD_FLAGS_KERNEL_LAUNCH_FLUSH_L3 : 0)),
          kernelEntryAddr, devAddrKernelArgs, devAddrkernelException, shire_mask, 0,
          reinterpret_cast<void*>(dummyKernelArgs), sizeof(dummyKernelArgs),
          device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED));
      }

      // Read back dummy Kernel Results from device
      for (int i = 0; i < totalKer / queueCountEmptyKer; i++) {
        vResultEmptyKer.resize(0x1000, 0);
        vResultStorageEmptyKer.push_back(std::move(vResultEmptyKer));
        auto hostVirtAddrRes = reinterpret_cast<uint64_t>(vResultStorageEmptyKer.back().data());
        auto hostPhysAddrRes = hostVirtAddrRes; // Should be handled in SysEmu, userspace should not fill this value
        streamEmptyKer.push_back(IDevOpsApiCmd::createDataReadCmd(
          i == 0, /* Barrier only for first read to make sure that all kernels execution done */
          devAddrResult[i], hostVirtAddrRes, hostPhysAddrRes, vResultStorageEmptyKer.back().size(),
          device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));
      }

      // Move stream of commands to streams_
      streams_.try_emplace(key(deviceIdxEmptyKer, queueIdxEmptyKer), std::move(streamEmptyKer));
    }
    readersStorageEmptyKer.push_back(std::move(readersEmptyKer));
  }

  executeAsync();

  TEST_VLOG(1) << "====> BACK TO BACK " << totalKer << " KERNEL LAUNCH (EMPTY KERNEL) DONE <====\n" << std::endl;
}
