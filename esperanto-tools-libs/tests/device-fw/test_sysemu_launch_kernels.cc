//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "device-fw-fixture.h"

#include <absl/flags/flag.h>
#include <array>
#include <chrono>
#include <cstdio>
#include <experimental/filesystem>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace et_runtime::device;
using namespace et_runtime;
namespace fs = std::experimental::filesystem;

ABSL_FLAG(std::string, kernels_dir, "",
          "Directory where different kernel ELF files are located");

TEST_F(DeviceFWTest, empty_kernel) {

  auto *target_device_ptr = &dev_->getTargetDevice();
  auto *target_device = dynamic_cast<device::RPCTarget *>(target_device_ptr);
  EXPECT_TRUE(target_device != nullptr);
  auto &mb_emu = target_device->mailboxDev();

  auto success = mb_emu.ready(std::chrono::seconds(20));
  EXPECT_TRUE(success);

  success = mb_emu.reset();
  EXPECT_TRUE(success);

  success = mb_emu.ready(std::chrono::seconds(20));
  EXPECT_TRUE(success);

  auto kernels_dir = absl::GetFlag(FLAGS_kernels_dir);
  fs::path empty_kernel = fs::path(kernels_dir)  / fs::path("empty.elf");

  auto load_res = dev_->moduleLoad("main", empty_kernel.string());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  layer_dynamic_info layer_info = {0};
  auto launch_res = dev_->rawLaunch(module_id, "main", &layer_info, sizeof(layer_info), nullptr);
  ASSERT_EQ(launch_res, etrtSuccess);
}

TEST_F(DeviceFWTest, beef_kernel) {
  auto *target_device_ptr = &dev_->getTargetDevice();
  auto *target_device = dynamic_cast<device::RPCTarget *>(target_device_ptr);
  EXPECT_TRUE(target_device != nullptr);
  auto &mb_emu = target_device->mailboxDev();

  auto success = mb_emu.ready(std::chrono::seconds(20));
  EXPECT_TRUE(success);

  success = mb_emu.reset();
  EXPECT_TRUE(success);

  success = mb_emu.ready(std::chrono::seconds(20));
  EXPECT_TRUE(success);

  auto kernels_dir = absl::GetFlag(FLAGS_kernels_dir);
  fs::path empty_kernel = fs::path(kernels_dir)  / fs::path("beef.elf");

  auto load_res = dev_->moduleLoad("main", empty_kernel.string());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  int array_size = 200;
  int size = sizeof(uint64_t) * array_size;
  void *dev_ptr = 0;
  auto status = dev_->malloc(&dev_ptr, size);
  ASSERT_EQ(status, etrtSuccess);

  layer_dynamic_info layer_info = {};
  layer_info.tensor_a = reinterpret_cast<uint64_t>(dev_ptr);
  layer_info.tensor_b = size;
  auto launch_res = dev_->rawLaunch(module_id, "main", &layer_info, sizeof(layer_info), nullptr);
  ASSERT_EQ(launch_res, etrtSuccess);

  std::vector<uint64_t> data(array_size, 0xEEEEEEEEEEEEEEEEULL);
  std::vector<uint64_t> refdata(array_size, 0xBEEFBEEFBEEFBEEFULL);

  auto res = dev_->memcpy(data.data(), dev_ptr, size, etrtMemcpyDeviceToHost);
  ASSERT_EQ(launch_res, etrtSuccess);
  ASSERT_THAT(data, ::testing::ElementsAreArray(refdata));
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  // Force logging in stderr and set min logging level
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv, {"test_pcie_launch_kernels.cc"});
  return RUN_ALL_TESTS();
}
