//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Core/CodeModule.h"
#include "Core/Device.h"
#include "Core/DeviceManager.h"
#include "Core/DeviceTarget.h"
#include "Core/ELFSupport.h"
#include "Core/ModuleManager.h"
#include "Device/TargetSysEmu.h"

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdio>
#include <experimental/filesystem>
#include <string>
#include <thread>

using namespace et_runtime;
using namespace et_runtime::device;
using namespace std;
namespace fs = std::experimental::filesystem;

namespace et_runtime {
namespace device {

DECLARE_string(dev_target);

}
} // namespace et_runtime

namespace {

TEST(ModuleManager, loadOnSysEMU) {
  // Send memory definition
  fs::path p = "/proc/self/exe";
  auto test_real_path = fs::read_symlink(p);
  auto dir_name = test_real_path.remove_filename();

  et_runtime::device::FLAGS_dev_target = "sysemu_grpc";
  auto device_manager = et_runtime::getDeviceManager();
  auto ret_value = device_manager->registerDevice(0);
  auto dev = ret_value.get();

  auto bootrom = dir_name / "bootrom.mem";
  // Start the simulator
  dev->setFWFilePaths({bootrom});
  ASSERT_EQ(dev->init(), etrtSuccess);

  auto conv_elf = dir_name / "convolution.elf";

  et_runtime::ModuleManager module_manager;

  auto module_res = module_manager.createModule("convolution.elf");
  auto conv_mid = get<0>(module_res);
  auto load_res = module_manager.loadOnDevice(conv_mid, conv_elf, dev.get());
  EXPECT_EQ(load_res.getError(), etrtSuccess);
  EXPECT_EQ(load_res.get(), conv_mid);

  auto softmax_elf = dir_name / "gpu_0_softmax_110.elf";
  auto module_res2 = module_manager.createModule("softmax");
  auto softmax_mid = get<0>(module_res2);
  load_res = module_manager.loadOnDevice(softmax_mid, softmax_elf, dev.get());
  EXPECT_EQ(load_res.getError(), etrtSuccess);
  EXPECT_EQ(load_res.get(), softmax_mid);

  //
  module_manager.destroyModule(conv_mid);
  module_manager.destroyModule(softmax_mid);

  // Stop the simulator
  EXPECT_EQ(etrtSuccess, dev->resetDevice());
}

extern std::string FLAGS_dev_target;

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

} // namespace
