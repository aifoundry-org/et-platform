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
#include "RPCDevice/TargetSysEmu.h"
#include "esperanto/runtime/Core/CommandLineOptions.h"
#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Core/DeviceManager.h"
#include "esperanto/runtime/Core/DeviceTarget.h"

#include <absl/flags/flag.h>
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

ABSL_FLAG(std::string, kernels_dir, "",
          "Directory where different kernel ELF files are located");

namespace {

class TestModule : public ::testing::Test {
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

// Load an ELF on SysEmu that has a single segment
TEST_F(TestModule, loadOnSysEMU_convolution_elf) {

  auto conv_elf = dir_name_ / "../convolution.elf";

  auto module = make_unique<Module>("convolution.elf", conv_elf.string());
  EXPECT_TRUE(module->readELF());

  EXPECT_TRUE(module->loadOnDevice(dev_.get()));
}

// Load an ELF on SysEmu that has multiple segments
TEST_F(TestModule, loadOnSysEMU_etsocmaxsplat_elf) {

  auto conv_elf = dir_name_ / "../etsocmaxsplat.elf";
  auto module = make_unique<Module>("etsocmaxsplat.elf", conv_elf.string());
  EXPECT_TRUE(module->readELF());

  EXPECT_TRUE(module->loadOnDevice(dev_.get()));

  auto entrypoint_res = module->onDeviceKernelEntryPoint("etsocmaxsplat");
  ASSERT_TRUE(entrypoint_res);
  auto address = entrypoint_res.get();
  EXPECT_EQ(address, 0x80050010A0);
}

// Load one of the test-kernels that require emplacing the buffer
// Load an ELF on SysEmu that has multiple segments
TEST_F(TestModule, loadOnSysEMU_empty_test_kernel) {
  auto kernels_dir = absl::GetFlag(FLAGS_kernels_dir);
  fs::path empty_kernel = fs::path(kernels_dir) / fs::path("empty.elf");

  auto module = make_unique<Module>("empty.elf", empty_kernel.string());
  EXPECT_TRUE(module->readELF());

  EXPECT_TRUE(module->loadOnDevice(dev_.get()));

  auto entrypoint_res = module->onDeviceKernelEntryPoint("empty.elf");
  ASSERT_TRUE(entrypoint_res);
}

} // namespace

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  // Force logging in stderr and set min logging level
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
