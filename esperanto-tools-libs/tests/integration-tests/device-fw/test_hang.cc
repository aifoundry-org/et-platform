//******************************************************************************
// Copyright (C) 2018,2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------


#include "device-fw-fixture.h"

#include <esperanto/runtime/Core/CommandLineOptions.h>
#include <esperanto/runtime/Core/Device.h>
#include <Core/KernelActions.h>
#include <esperanto/runtime/EsperantoRuntime.h>

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
#include <fstream>

using namespace et_runtime::device;
using namespace et_runtime;
namespace fs = std::experimental::filesystem;

ABSL_FLAG(std::string, kernels_dir, "",
          "Directory where different kernel ELF files are located");

// Hang kernel test
// Steps:
// Monitor kernel state (should be unuswed)
// Load module and launch kernel w/o blocking
// Monitor state again (should be running)
// Since it will remain running forever, send an abort
// Monitor state again, it should be unused
// TBD: Launch beef kernel afterwards, once SW-1373 is fixed.
TEST_F(DeviceFWTest, hang_kernel) {

  // Get kernel info
  auto kernels_dir = absl::GetFlag(FLAGS_kernels_dir);
  fs::path hang_kernel_loc = fs::path(kernels_dir) / fs::path("hang.elf");
  std::cout <<  "Running: " << hang_kernel_loc.string() << "\n";

  auto &registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel("main", {Kernel::ArgType::T_layer_dynamic_info},
                                              hang_kernel_loc.string());
  ASSERT_TRUE((bool)register_res);
  auto &kernel = std::get<1>(register_res.get());

  // First make sure that kernel is on the unused state
  KernelActions kernel_actions;
  const TimeDuration wait_interval = std::chrono::seconds(5);
  auto kernel_state_res = kernel_actions.state(&dev_->defaultStream());
  std::this_thread::sleep_for(wait_interval);
  ASSERT_EQ(kernel_state_res.get(), ::device_api::DEV_API_KERNEL_STATE_UNUSED);

  // Load the kernel on the device
  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  auto module_id = load_res.get();

  Kernel::layer_dynamic_info_t layer_info = {};
  // Set the kernel launch arguments
  Kernel::LaunchArg arg;
  arg.type = Kernel::ArgType::T_layer_dynamic_info;
  arg.value.layer_dynamic_info = layer_info;
  auto args = std::vector<Kernel::LaunchArg>({arg});
  // Create a KernelLaunch object from this kernel
  auto launch = kernel.createKernelLaunch(args);

  // Launch w/o blocking so you can query the state
  auto launch_res = launch->launchNonBlocking(&dev_->defaultStream());
  ASSERT_EQ(launch_res, etrtSuccess);

  // Poll until kernel state becomes running.
  // It is momentarily in the launched state, then goes to running.
  // If it takes too long to go to the running state, fail.
  for (uint64_t poll=0; poll < 5; poll++) {
    kernel_state_res = kernel_actions.state(&dev_->defaultStream());
    if (kernel_state_res.get() ==  ::device_api::DEV_API_KERNEL_STATE_RUNNING) {
      break;
    }
    std::this_thread::sleep_for(wait_interval);
  }

  ASSERT_EQ(kernel_state_res.get(), ::device_api::DEV_API_KERNEL_STATE_RUNNING);

  // Abort kernel and then poll until it becomes unused again.
  // If it takes too long, fail
  kernel_actions.abort(&dev_->defaultStream());
  for (uint64_t poll=0; poll < 5; poll++) {
    kernel_state_res = kernel_actions.state(&dev_->defaultStream());
    std::this_thread::sleep_for(wait_interval);
    if (kernel_state_res.get() ==  ::device_api::DEV_API_KERNEL_STATE_UNUSED) {
      break;
    }
  }

  ASSERT_EQ(kernel_state_res.get(), ::device_api::DEV_API_KERNEL_STATE_UNUSED);


  // Fixme: SW-1373. Unloading does not free up the memory
  // auto unload_res = registry.moduleUnload(kernel.moduleID(), dev_.get());
  // ASSERT_EQ(unload_res, etrtSuccess);

  // Fixme:
  // If we do not unloaded the next kernel will go to the same address
  // and there will be a conflict. Once this or SW-1373 is fixed, uncomment the rest
  /*
  fs::path beef_kernel_path = fs::path(kernels_dir)  / fs::path("beef.elf");

  register_res = registry.registerKernel("beef_main", {Kernel::ArgType::T_layer_dynamic_info},
                                              beef_kernel_path.string());
  ASSERT_TRUE((bool)register_res);
  auto &beef_kernel = std::get<1>(register_res.get());

  load_res = registry.moduleLoad(beef_kernel.moduleID(), dev_.get());
  ASSERT_EQ(load_res.getError(), etrtSuccess);
  module_id = load_res.get();

  int array_size = 200;
  int size = sizeof(uint64_t) * array_size;
  void *dev_ptr = 0;
  auto status = dev_->mem_manager().malloc(&dev_ptr, size);
  ASSERT_EQ(status, etrtSuccess);

  layer_info.tensor_a = reinterpret_cast<uint64_t>(dev_ptr);
  layer_info.tensor_b = size;
  arg.type = Kernel::ArgType::T_layer_dynamic_info;
  arg.value.layer_dynamic_info = layer_info;
  args = std::vector<Kernel::LaunchArg>({arg});
  launch = kernel.createKernelLaunch(args);
  launch_res = launch->launchBlocking(&dev_->defaultStream());
  ASSERT_EQ(launch_res, etrtSuccess);

  std::vector<uint64_t> data(array_size, 0xEEEEEEEEEEEEEEEEULL);
  std::vector<uint64_t> refdata(array_size, 0xBEEFBEEFBEEFBEEFULL);
  auto res = dev_->memcpy(data.data(), dev_ptr, size, etrtMemcpyDeviceToHost);
  ASSERT_EQ(launch_res, etrtSuccess);
  ASSERT_THAT(data, ::testing::ElementsAreArray(refdata));
  */
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
