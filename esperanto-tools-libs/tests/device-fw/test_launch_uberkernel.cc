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

#include "esperanto/runtime/CodeManagement/CodeRegistry.h"

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

TEST_F(DeviceFWTest, launch_uberkernel) {
  auto kernels_dir = absl::GetFlag(FLAGS_kernels_dir);
  fs::path uberkernel = fs::path(kernels_dir) / fs::path("uberkernel.elf");
  auto &registry = CodeRegistry::registry();

  // Register a 2 layer uberkernel
  auto register_res =
      registry.registerUberKernel("main",
                                  {{Kernel::ArgType::T_layer_dynamic_info},
                                   {Kernel::ArgType::T_layer_dynamic_info}},
                                  uberkernel.string());
  ASSERT_TRUE((bool)register_res);
  auto &kernel = std::get<1>(register_res.get());

  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  int array_size = 200;
  int size = sizeof(uint64_t) * array_size;
  void *dev_ptr = 0;
  auto status = dev_->mem_manager().malloc(&dev_ptr, size);
  ASSERT_EQ(status, etrtSuccess);

  Kernel::layer_dynamic_info_t layer_info = {};
  layer_info.tensor_a = reinterpret_cast<uint64_t>(dev_ptr);
  layer_info.tensor_b = size;
  Kernel::LaunchArg arg;
  arg.type = Kernel::ArgType::T_layer_dynamic_info;
  arg.value.layer_dynamic_info = layer_info;

  int array_size_2 = 400;
  size = sizeof(uint64_t) * array_size_2;
  void *dev_ptr_2 = 0;
  status = dev_->mem_manager().malloc(&dev_ptr_2, size);
  ASSERT_EQ(status, etrtSuccess);

  layer_info = {};
  layer_info.tensor_a = reinterpret_cast<uint64_t>(dev_ptr_2);
  layer_info.tensor_b = size;
  Kernel::LaunchArg arg2;
  arg2.type = Kernel::ArgType::T_layer_dynamic_info;
  arg2.value.layer_dynamic_info = layer_info;

  auto args = std::vector<std::vector<Kernel::LaunchArg>>({{arg}, {arg2}});
  auto launch = kernel.createKernelLaunch(args);
  auto launch_res = launch->launchBlocking(&dev_->defaultStream());
  ASSERT_EQ(launch_res, etrtSuccess);

  ///  Check writing from the first layer
  {
    std::vector<uint64_t> data(array_size, 0xEEEEEEEEEEEEEEEEULL);
    std::vector<uint64_t> refdata(array_size, 0xBEEFBEEFBEEFBEEFULL);
    auto res = dev_->memcpy(data.data(), dev_ptr, array_size * sizeof(uint64_t),
                            etrtMemcpyDeviceToHost);
    ASSERT_EQ(res, etrtSuccess);
    ASSERT_THAT(data, ::testing::ElementsAreArray(refdata));
  }

  ///  Check writing from the second layer
  {
    std::vector<uint64_t> data(array_size_2, 0xEEEEEEEEEEEEEEEEULL);
    std::vector<uint64_t> refdata(array_size_2, 0xBEEFBEEFBEEFBEEFULL);
    auto res =
        dev_->memcpy(data.data(), dev_ptr_2, array_size_2 * sizeof(uint64_t),
                     etrtMemcpyDeviceToHost);
    ASSERT_EQ(res, etrtSuccess);
    ASSERT_THAT(data, ::testing::ElementsAreArray(refdata));
  }
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  // Force logging in stderr and set min logging level
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv,
                                      {"test_launch_uberkernel.cc"});
  return RUN_ALL_TESTS();
}
