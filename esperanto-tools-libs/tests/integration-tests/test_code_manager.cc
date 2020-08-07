//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "CodeManagement/CodeModule.h"
#include "CodeManagement/ELFSupport.h"
#include "CodeManagement/ModuleManager.h"
#include "esperanto/runtime/CodeManagement/CodeRegistry.h"
#include "esperanto/runtime/Core/CommandLineOptions.h"
#include "esperanto/runtime/Core/DeviceManager.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <experimental/filesystem>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <thread>

using namespace et_runtime;
using namespace et_runtime::device;
using namespace std;
namespace fs = std::experimental::filesystem;

namespace {

class TestCodeRegistry : public ::testing::Test {
public:
  void SetUp() override {
    // Send memory definition
    fs::path p = "/proc/self/exe";
    auto test_real_path = fs::read_symlink(p);
    dir_name_ = test_real_path.remove_filename();

    absl::SetFlag(&FLAGS_dev_target, DeviceTargetOption("sysemu_grpc"));
    auto device_manager = et_runtime::getDeviceManager();
    auto ret_value = device_manager->registerDevice(0);
    dev_ = ret_value.get();

    // Start the simulator
    ASSERT_EQ(dev_->init(), etrtSuccess);
  }

  void TearDown() override {

    // Stop the simulator
    EXPECT_EQ(etrtSuccess, dev_->resetDevice());
  }

  fs::path dir_name_;
  std::shared_ptr<Device> dev_;
};

// Register a single Kernel. The ELF is expected to contain a single kernel
TEST_F(TestCodeRegistry, registerKernel) {

  // FIXME SW-1369 do not consume the downloaded ELF file
  // We expect that the elf we test with is installed next to the test-binary
  // Find the absolute pasth of the test binary
  fs::path p = "/proc/self/exe";
  auto test_real_path = fs::read_symlink(p);
  auto dir_name = test_real_path.remove_filename();
  auto conv_elf = dir_name / "convolution.elf";

  auto res = dev_->codeRegistry().registerKernel(
      "convolution", {Kernel::ArgType::T_layer_dynamic_info},
      conv_elf.string());
  ASSERT_TRUE(res);

  auto kernel_id = std::get<0>(res.get());
  ASSERT_TRUE(kernel_id != 0);
}

// Register a single Uber Kernel. The ELF is expected to contain a single kernel
TEST_F(TestCodeRegistry, registerUberKernel) {

  // FIXME SW-1369 do not consume the downloaded ELF file
  // We expect that the elf we test with is installed next to the test-binary
  // Find the absolute pasth of the test binary
  fs::path p = "/proc/self/exe";
  auto test_real_path = fs::read_symlink(p);
  auto dir_name = test_real_path.remove_filename();
  // FIXME Use a real Uber Kernel
  auto conv_elf = dir_name / "convolution.elf";

  auto res = dev_->codeRegistry().registerUberKernel(
      "convolutio_2", {{Kernel::ArgType::T_layer_dynamic_info}},
      conv_elf.string());
  ASSERT_TRUE(res);

  auto kernel_id = std::get<0>(res.get());
  ASSERT_TRUE(kernel_id != 0);

  auto find_kernel = Kernel::findKernel(kernel_id);
  ASSERT_TRUE((bool)find_kernel);
}

} // namespace

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  // Force logging in stderr and set min logging level
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
