//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/Core/CommandLineOptions.h"
#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Core/DeviceManager.h"
#include "esperanto/runtime/Core/Profiler.h"
#include "esperanto/runtime/CodeManagement/CodeRegistry.h"

#include "Tracing/etrt-trace.pb.h"

#include <google/protobuf/util/delimited_message_util.h>

#include <chrono>
#include <cstdio>
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <string>
#include <thread>
#include <fcntl.h>
#include <experimental/filesystem>

using namespace et_runtime;
using namespace et_runtime::device;
namespace fs = std::experimental::filesystem;

ABSL_FLAG(std::string, kernels_dir, "",
          "Directory where different kernel ELF files are located");

class TestProfiler : public ::testing::Test {
public:
  void SetUp() override {
    auto device_manager = et_runtime::getDeviceManager();
    auto ret_value = device_manager->registerDevice(0);
    dev_ = ret_value.get();

    // Start the simulator
    ASSERT_EQ(dev_->init(), etrtSuccess);
  }

  void do_something_useful() {
    // register kernel
    fs::path trace_kernel = fs::path(absl::GetFlag(FLAGS_kernels_dir)) /
                          fs::path("trace_ring_buffer.elf");
    auto &registry = dev_->codeRegistry();

    auto register_res = registry.registerKernel("main", {Kernel::ArgType::T_layer_dynamic_info}, trace_kernel.string());
    ASSERT_TRUE((bool)register_res);
    auto& kernel = std::get<1>(register_res.get());

    auto load_res = registry.moduleLoad(kernel.moduleID(), dev_.get());
    ASSERT_EQ(load_res.getError(), etrtSuccess);

    // Prepare kernel launch
    Kernel::layer_dynamic_info_t layer_info = {0};
    Kernel::LaunchArg arg;
    arg.type = Kernel::ArgType::T_layer_dynamic_info;
    arg.value.layer_dynamic_info = layer_info;
    auto args = std::vector<Kernel::LaunchArg>({arg});
    auto launch = kernel.createKernelLaunch(args);

    // Lauch the kernel
    auto launch_res = launch->launchBlocking(&dev_->defaultStream());
    ASSERT_EQ(launch_res, etrtSuccess);
  }

  void check_trace() {
    auto path = absl::GetFlag(FLAGS_etrt_trace);
    ASSERT_TRUE(!path.empty());

    int infd = open(path.c_str(), O_RDONLY);

    google::protobuf::io::FileInputStream fin(infd);    
    ASSERT_EQ(fin.GetErrno(), 0);

    bool contains_kernel_create = false;
    bool contains_kernel_launch = false;

    bool keep_reading = true;
    while (keep_reading) {
      et_runtime::tracing::RuntimeTraceEntry mentry;
      keep_reading = google::protobuf::util::ParseDelimitedFromZeroCopyStream(&mentry, &fin, nullptr);
      
      if (mentry.has_codemanager_create_kernel_e())
      {
        contains_kernel_create = true;
      }
      else if (mentry.has_codemanager_kernel_launch_e())
      {
        contains_kernel_launch = true;
      }
    }
    ASSERT_TRUE(contains_kernel_create);
    ASSERT_TRUE(contains_kernel_launch);

    fin.Close();
    close(infd);
  }

  void TearDown() override {
    // Stop the simulator
    EXPECT_EQ(etrtSuccess, dev_->resetDevice());
  }
  
  std::shared_ptr<Device> dev_;
};

TEST_F(TestProfiler, profiler) {
  // initialize profiler for device dev_
  Profiler profiler(*dev_);

  // start collecting events
  auto result = profiler.start();
  EXPECT_EQ(result, etrtSuccess);

  // do something in the device that generates
  // events
  {
    do_something_useful();
  }

  // stop collecting events
  result = profiler.stop();
  EXPECT_EQ(result, etrtSuccess);

  // flush data to protobuf
  result = profiler.flush();
  EXPECT_EQ(result, etrtSuccess);

  check_trace();
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
