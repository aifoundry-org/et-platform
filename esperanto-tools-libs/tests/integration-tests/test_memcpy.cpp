//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "runtime/IRuntime.h"

#include "common/Constants.h"
#include <device-layer/IDeviceLayer.h>
#include <experimental/filesystem>
#include <fstream>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <ios>

namespace {
constexpr uint64_t kSysEmuMaxCycles = std::numeric_limits<uint64_t>::max();
constexpr uint64_t kSysEmuMinionShiresMask = 0x1FFFFFFFFu;
class TestMemcpy : public ::testing::Test {
public:
  void SetUp() override {
    emu::SysEmuOptions sysEmuOptions;
    sysEmuOptions.bootromTrampolineToBL2ElfPath = BOOTROM_TRAMPOLINE_TO_BL2_ELF;
    sysEmuOptions.spBL2ElfPath = BL2_ELF;
    sysEmuOptions.machineMinionElfPath = MACHINE_MINION_ELF;
    sysEmuOptions.masterMinionElfPath = MASTER_MINION_ELF;
    sysEmuOptions.workerMinionElfPath = WORKER_MINION_ELF;
    sysEmuOptions.executablePath = std::string(SYSEMU_INSTALL_DIR) + "sys_emu";
    sysEmuOptions.runDir = std::experimental::filesystem::current_path();
    sysEmuOptions.maxCycles = kSysEmuMaxCycles;
    sysEmuOptions.minionShiresMask = kSysEmuMinionShiresMask;
    sysEmuOptions.puUart0Path = sysEmuOptions.runDir + "/pu_uart0_tx.log";
    sysEmuOptions.puUart1Path = sysEmuOptions.runDir + "/pu_uart1_tx.log";
    sysEmuOptions.spUart0Path = sysEmuOptions.runDir + "/spio_uart0_tx.log";
    sysEmuOptions.spUart1Path = sysEmuOptions.runDir + "/spio_uart1_tx.log";
    sysEmuOptions.startGdb = false;

    deviceLayer_ = dev::IDeviceLayer::createSysEmuDeviceLayer(sysEmuOptions);
    runtime_ = rt::IRuntime::create(deviceLayer_.get());
    devices_ = runtime_->getDevices();
    ASSERT_GE(devices_.size(), 1);
  }

  void TearDown() override {
    runtime_.reset();
  }

  rt::RuntimePtr runtime_;
  std::unique_ptr<dev::IDeviceLayer> deviceLayer_;
  std::vector<rt::DeviceId> devices_;
};

// Load and removal of a single kernel.
TEST_F(TestMemcpy, SimpleMemcpy) {

  int numElems = 1024 * 1024 * 10;
  auto dev = devices_[0];
  auto stream = runtime_->createStream(dev);
  auto random_trash = std::vector<int>();

  for (int i = 0; i < numElems; ++i) {
    random_trash.emplace_back(rand());
  }

  // alloc memory in device
  auto sizeBytes = random_trash.size() * sizeof(int);
  auto d_buffer = runtime_->mallocDevice(dev, sizeBytes);

  // copy from host to device and from device to result buffer host; check they are equal
  runtime_->memcpyHostToDevice(stream, reinterpret_cast<std::byte*>(random_trash.data()), d_buffer, sizeBytes);
  auto result = std::vector<int>(numElems);
  ASSERT_NE(random_trash, result);

  runtime_->memcpyDeviceToHost(stream, d_buffer, reinterpret_cast<std::byte*>(result.data()), sizeBytes);
  runtime_->waitForStream(stream);

  ASSERT_EQ(random_trash, result);
}

} // namespace

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
