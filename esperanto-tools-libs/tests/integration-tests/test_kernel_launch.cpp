//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/Common/ProjectAutogen.h"
#include "runtime/IRuntime.h"
#include <device-layer/IDeviceLayer.h>
#include <esperanto/runtime/Core/CommandLineOptions.h>
#include <experimental/filesystem>
#include <fstream>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <ios>
#include <thread>

ABSL_FLAG(std::string, kernels_dir, "", "Directory where different kernel ELF files are located");
namespace {
constexpr uint64_t kSysEmuMaxCycles = std::numeric_limits<uint64_t>::max();
constexpr uint64_t kSysEmuMinionShiresMask = 0x1FFFFFFFFu;
} // namespace

namespace fs = std::experimental::filesystem;
TEST(KernelLaunch, add_2_vectors) {
  auto kernel_file = std::ifstream(absl::GetFlag(FLAGS_kernels_dir) + "/" + "add_vector.elf", std::ios::binary);
  ASSERT_TRUE(kernel_file.is_open());

  auto iniF = kernel_file.tellg();
  kernel_file.seekg(0, std::ios::end);
  auto endF = kernel_file.tellg();
  auto size = endF - iniF;
  kernel_file.seekg(0, std::ios::beg);

  std::vector<std::byte> kernelContent(size);
  kernel_file.read(reinterpret_cast<char*>(kernelContent.data()), size);

  emu::SysEmuOptions sysEmuOptions;
  sysEmuOptions.bootromTrampolineToBL2ElfPath = BOOTROM_TRAMPOLINE_TO_BL2_ELF;
  sysEmuOptions.spBL2ElfPath = BL2_NEW_ELF;
  sysEmuOptions.machineMinionElfPath = MACHINE_MINION_ELF;
  sysEmuOptions.masterMinionElfPath = MASTER_MINION_NEW_ELF;
  sysEmuOptions.workerMinionElfPath = WORKER_MINION_NEW_ELF;
  sysEmuOptions.executablePath = std::string(SYSEMU_INSTALL_DIR) + "sys_emu";
  sysEmuOptions.runDir = std::experimental::filesystem::current_path();
  sysEmuOptions.maxCycles = kSysEmuMaxCycles;
  sysEmuOptions.minionShiresMask = kSysEmuMinionShiresMask;
  sysEmuOptions.puUart0Path = sysEmuOptions.runDir + "/pu_uart0_tx.log";
  sysEmuOptions.puUart1Path = sysEmuOptions.runDir + "/pu_uart1_tx.log";
  sysEmuOptions.spUart0Path = sysEmuOptions.runDir + "/spio_uart0_tx.log";
  sysEmuOptions.spUart1Path = sysEmuOptions.runDir + "/spio_uart1_tx.log";
  sysEmuOptions.startGdb = false;

  auto deviceLayer = dev::IDeviceLayer::createSysEmuDeviceLayer(sysEmuOptions);

  //#TODO find out if there is a define or something which indicates sysemu or device in tests
  auto runtime = rt::IRuntime::create(deviceLayer.get());
  auto devices = runtime->getDevices();
  auto dev = devices[0];

  auto kernelId = runtime->loadCode(dev, kernelContent.data(), kernelContent.size());

  std::vector<int> vA, vB, vResult;
  int numElems = 10496;
  for (int i = 0; i < numElems; ++i) {
    vA.emplace_back(rand());
    vB.emplace_back(rand());
    vResult.emplace_back(vA.back() + vB.back());
  }
  auto bufferSize = numElems * sizeof(int);
  auto bufA = runtime->mallocDevice(devices[0], bufferSize);
  auto bufB = runtime->mallocDevice(devices[0], bufferSize);
  auto bufResult = runtime->mallocDevice(devices[0], bufferSize);

  auto stream = runtime->createStream(dev);
  runtime->memcpyHostToDevice(stream, vA.data(), bufA, bufferSize);
  runtime->memcpyHostToDevice(stream, vB.data(), bufB, bufferSize);

  struct Parameters {
    void* vA;
    void* vB;
    void* vResult;
    int numElements;
  } parameters{bufA, bufB, bufResult, numElems};

  std::vector<int> resultFromDevice(numElems);
  for (int i = 0; i < numElems; ++i) {
    resultFromDevice[i] = 0xF;
  }
  runtime->memcpyHostToDevice(stream, resultFromDevice.data(), bufResult, bufferSize);
  runtime->kernelLaunch(stream, kernelId, &parameters, sizeof(parameters), 0x1);
  for (int i = 0; i < numElems; ++i) {
    resultFromDevice[i] = 0x0;
  }
  runtime->memcpyDeviceToHost(stream, bufResult, resultFromDevice.data(), bufferSize);
  runtime->waitForStream(stream);
  EXPECT_EQ(resultFromDevice, vResult);
}

TEST(KernelLaunch, memset) {
  auto kernel_file = std::ifstream(absl::GetFlag(FLAGS_kernels_dir) + "/" + "memset.elf", std::ios::binary);
  ASSERT_TRUE(kernel_file.is_open());

  auto iniF = kernel_file.tellg();
  kernel_file.seekg(0, std::ios::end);
  auto endF = kernel_file.tellg();
  auto size = endF - iniF;
  kernel_file.seekg(0, std::ios::beg);

  std::vector<std::byte> kernelContent(size);
  kernel_file.read(reinterpret_cast<char*>(kernelContent.data()), size);

  emu::SysEmuOptions sysEmuOptions;
  sysEmuOptions.bootromTrampolineToBL2ElfPath = BOOTROM_TRAMPOLINE_TO_BL2_ELF;
  sysEmuOptions.spBL2ElfPath = BL2_NEW_ELF;
  sysEmuOptions.machineMinionElfPath = MACHINE_MINION_ELF;
  sysEmuOptions.masterMinionElfPath = MASTER_MINION_NEW_ELF;
  sysEmuOptions.workerMinionElfPath = WORKER_MINION_NEW_ELF;
  sysEmuOptions.executablePath = std::string(SYSEMU_INSTALL_DIR) + "sys_emu";
  sysEmuOptions.runDir = std::experimental::filesystem::current_path();
  sysEmuOptions.maxCycles = kSysEmuMaxCycles;
  sysEmuOptions.minionShiresMask = kSysEmuMinionShiresMask;
  sysEmuOptions.puUart0Path = sysEmuOptions.runDir + "/pu_uart0_tx.log";
  sysEmuOptions.puUart1Path = sysEmuOptions.runDir + "/pu_uart1_tx.log";
  sysEmuOptions.spUart0Path = sysEmuOptions.runDir + "/spio_uart0_tx.log";
  sysEmuOptions.spUart1Path = sysEmuOptions.runDir + "/spio_uart1_tx.log";
  sysEmuOptions.startGdb = false;

  auto deviceLayer = dev::IDeviceLayer::createSysEmuDeviceLayer(sysEmuOptions);

  //#TODO find out if there is a define or something which indicates sysemu or device in tests
  auto runtime = rt::IRuntime::create(deviceLayer.get());
  auto devices = runtime->getDevices();
  auto dev = devices[0];

  auto kernelId = runtime->loadCode(dev, kernelContent.data(), kernelContent.size());
  int numElems = 10496;
  std::vector<int> vR(numElems);
  auto bufferSize = numElems * sizeof(int);
  auto bufResult = runtime->mallocDevice(dev, bufferSize);

  int numShires = 16;
  int value = 0xDE;
  struct Parameters {
    void* a;
    int value;
    int numElements;
    int numShires;
  } parameters{bufResult, value, numElems, numShires};

  auto stream = runtime->createStream(dev);
  runtime->kernelLaunch(stream, kernelId, &parameters, sizeof(parameters), (1 << numShires) - 1);
  runtime->memcpyDeviceToHost(stream, bufResult, vR.data(), bufferSize);
  runtime->waitForStream(stream);
  for (auto v : vR) {
    EXPECT_EQ(v, value);
  }
  DLOG(INFO) << "End test";
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  // Force logging in stderr and set min logging level
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv, {"test_kernel_launch.cpp"});
  return RUN_ALL_TESTS();
}