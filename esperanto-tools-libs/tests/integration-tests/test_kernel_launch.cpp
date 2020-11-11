//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "runtime/IRuntime.h"
#include <esperanto/runtime/Core/CommandLineOptions.h>
#include <experimental/filesystem>
#include <fstream>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <ios>
#include <thread>
ABSL_FLAG(std::string, kernels_dir, "", "Directory where different kernel ELF files are located");

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

  //#TODO find out if there is a define or something which indicates sysemu or device in tests
  auto runtime = rt::IRuntime::create(rt::IRuntime::Kind::SysEmu);
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
  runtime->memcpyHostToDevice(stream, reinterpret_cast<std::byte*>(vA.data()), bufA, bufferSize);
  runtime->memcpyHostToDevice(stream, reinterpret_cast<std::byte*>(vB.data()), bufB, bufferSize);

  struct Parameters {
    std::byte* vA;
    std::byte* vB;
    std::byte* vResult;
    int numElements;
  } parameters{bufA, bufB, bufResult, numElems};

  std::vector<int> resultFromDevice(numElems);
  for (int i = 0; i < numElems; ++i) {
    resultFromDevice[i] = 0xF;
  }
  runtime->memcpyHostToDevice(stream, (std::byte*)resultFromDevice.data(), bufResult, bufferSize);
  runtime->kernelLaunch(stream, kernelId, (std::byte*)&parameters, sizeof(parameters), 0x1);
  for (int i = 0; i < numElems; ++i) {
    resultFromDevice[i] = 0x0;
  }
  runtime->memcpyDeviceToHost(stream, bufResult, (std::byte*)resultFromDevice.data(), bufferSize);
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

  //#TODO find out if there is a define or something which indicates sysemu or device in tests
  auto runtime = rt::IRuntime::create(rt::IRuntime::Kind::SysEmu);
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
    std::byte* a;
    int value;
    int numElements;
    int numShires;
  } parameters{bufResult, value, numElems, numShires};

  auto stream = runtime->createStream(dev);
  runtime->kernelLaunch(stream, kernelId, (std::byte*)&parameters, sizeof(parameters), (1 << numShires) - 1);
  runtime->memcpyDeviceToHost(stream, bufResult, (std::byte*)vR.data(), bufferSize);
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