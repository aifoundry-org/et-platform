//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Core/CommandLineOptions.h"
#include "Core/Device.h"
#include "Device/PCIeDevice.h"

#include <absl/flags/flag.h>
#include <array>
#include <chrono>
#include <cstdio>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <experimental/filesystem>

using namespace et_runtime::device;
using namespace et_runtime;
namespace fs = std::experimental::filesystem;

ABSL_FLAG(std::string, kernels_dir, "",
          "Directory where different kernel ELF files are located");

class PCIEKernelLaunchTest : public ::testing::Test {
protected:
  void SetUp() override {
    absl::SetFlag(&FLAGS_dev_target, DeviceTargetOption("pcie"));
    dev_ = std::make_shared<Device>(0);
    auto error = dev_->init();
    ASSERT_EQ(error, etrtSuccess);
  }

  void TearDown() override {
    dev_.reset();
  }
  std::shared_ptr<Device> dev_;
};

TEST_F(PCIEKernelLaunchTest, empty_kernel) {
  auto kernels_dir = absl::GetFlag(FLAGS_kernels_dir);
  fs::path empty_kernel = fs::path(kernels_dir)  / fs::path("empty.elf");

  auto load_res = dev_->moduleLoad("main", empty_kernel.string());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  layer_dynamic_info layer_info = {0};
  auto launch_res = dev_->rawLaunch(module_id, "main", &layer_info, sizeof(layer_info), nullptr);
  ASSERT_EQ(launch_res, etrtSuccess);
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
