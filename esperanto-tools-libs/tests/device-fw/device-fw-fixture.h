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
#include "Core/CommandLineOptions.h"
#include "Core/Device.h"
#include "Core/DeviceManager.h"
#include "Core/DeviceTarget.h"
#include "Core/ELFSupport.h"
#include "Core/ModuleManager.h"
#include "Device/TargetSysEmu.h"

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

class DeviceFWTest : public ::testing::Test {

protected:
  void SetUp() override {
    fs::path p = "/proc/self/exe";
    auto test_real_path = fs::read_symlink(p);
    auto dir_name = test_real_path.remove_filename();

    absl::SetFlag(&FLAGS_dev_target, DeviceTargetOption("sysemu_grpc"));
    auto fw_type = absl::GetFlag(FLAGS_fw_type);
    ASSERT_STREQ(fw_type.type.c_str(), "device-fw");
    absl::SetFlag(&FLAGS_fw_type, FWType("device-fw"));
    auto device_manager = et_runtime::getDeviceManager();
    auto ret_value = device_manager->registerDevice(0);
    dev_ = ret_value.get();

    auto worker_minion = absl::GetFlag(FLAGS_worker_minion_elf);
    auto machine_minion = absl::GetFlag(FLAGS_machine_minion_elf);
    auto master_minion = absl::GetFlag(FLAGS_master_minion_elf);

    // Start the simulator and load device-fw in memory
    dev_->setFWFilePaths({master_minion, machine_minion, worker_minion});
    ASSERT_EQ(dev_->init(), etrtSuccess);

    auto *target_device_ptr = &dev_->getTargetDevice();
    auto *target_device = dynamic_cast<device::RPCTarget *>(target_device_ptr);
    ASSERT_TRUE(target_device != nullptr);

    // Boot device-fw
    target_device->boot(0x8000001000);
    auto res = target_device->waitForHostInterrupt(std::chrono::seconds(30));
    ASSERT_TRUE(res);
  }

  void TearDown() override {

  // Stop the simulator
  EXPECT_EQ(etrtSuccess, dev_->resetDevice());
  }

  std::shared_ptr<Device> dev_;

};
