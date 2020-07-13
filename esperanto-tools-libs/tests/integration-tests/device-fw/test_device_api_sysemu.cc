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

#include "Core/KernelActions.h"
#include "esperanto/runtime/CodeManagement/CodeRegistry.h"
#include "esperanto/runtime/CodeManagement/Kernel.h"
#include "esperanto/runtime/Core/DeviceHelpers.h"
#include "esperanto/runtime/Core/VersionCheckers.h"
#include "esperanto/runtime/Support/Logging.h"

#include <thread>
#include <chrono>

ABSL_FLAG(std::string, kernels_dir, "",
          "Directory where different kernel ELF files are located");

using namespace et_runtime;


///  FIXME SW-1364
// Test setting the master minion logging levels
TEST_F(DeviceFWTest, DeviceAPI_SetLogLevelWorker) {
  SetWorkerLogLevel worker_level(*dev_);
  auto success = worker_level.set_level_critical();
  ASSERT_TRUE(success);
  success = worker_level.set_level_error();
  ASSERT_TRUE(success);
  success = worker_level.set_level_warning();
  ASSERT_TRUE(success);
  success = worker_level.set_level_info();
  ASSERT_TRUE(success);
  success = worker_level.set_level_debug();
  ASSERT_TRUE(success);
  success = worker_level.set_level_trace();
  ASSERT_TRUE(success);
}

// Test that will launch a kernel that is expected to
TEST_F(DeviceFWTest, DeviceAPI_CheckKernelStatus) {
  auto kernels_dir = absl::GetFlag(FLAGS_kernels_dir);
  fs::path beef_kernel = fs::path(kernels_dir) / fs::path("hang.elf");
  auto &registry = dev_->codeRegistry();

  auto register_res = registry.registerKernel(
      "main", {Kernel::ArgType::T_layer_dynamic_info}, beef_kernel.string());
  ASSERT_TRUE((bool)register_res);
  auto &kernel = std::get<1>(register_res.get());
  auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());

  KernelActions kernel_actions;

  // FIXME SW-1279 use the hang kernel to test correctly that we can
  // abort a hang kernel and start runnign a new one.
  // The problem is currently all DeviceAPI calls are practically blocking
  // and wait for a reply. Fix this test is resolved


  // Kernel::layer_dynamic_info_t layer_info = {0};
  // Kernel::LaunchArg arg;
  // arg.type = Kernel::ArgType::T_layer_dynamic_info;
  // arg.value.layer_dynamic_info = layer_info;
  // auto args = std::vector<Kernel::LaunchArg>({arg});

  // auto launch_res = kernel_actions.launchNonBlocking(&dev_->defaultStream(),
  // kernel, args);

  // ASSERT_EQ(launch_res, etrtSuccess);

  auto kernel_state_res = kernel_actions.state(&dev_->defaultStream());
  ASSERT_EQ(kernel_state_res.getError(), etrtSuccess);

  RTINFO << "Kernel Status: " << (int)kernel_state_res.get() << "\n";
  ASSERT_EQ(kernel_state_res.get(),
            ::device_api::non_privileged::DEV_API_KERNEL_STATE_UNUSED);

  auto kernel_abort_res = kernel_actions.abort(&dev_->defaultStream());
  ASSERT_EQ(kernel_abort_res.getError(), etrtSuccess);

  RTINFO << "Kernel abort: " << (int)kernel_abort_res.get() << "\n";
  ASSERT_EQ(
      kernel_abort_res.get(),
      ::device_api::non_privileged::DEV_API_KERNEL_ABORT_RESPONSE_RESULT_OK);
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
