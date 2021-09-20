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

#include "RuntimeImp.h"
#include "common/Constants.h"
#include <device-layer/IDeviceLayer.h>
#include <experimental/filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logger.h>
#include <ios>
#include <random>

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
    auto imp = static_cast<rt::RuntimeImp*>(runtime_.get());
    imp->setMemoryManagerDebugMode(devices_[0], true);
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
  std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution dis;

  auto numElems = 1024 * 1024 * 10;
  auto dev = devices_[0];
  auto stream = runtime_->createStream(dev);
  auto random_trash = std::vector<int>();

  for (int i = 0; i < numElems; ++i) {
    random_trash.emplace_back(dis(gen));
  }

  // alloc memory in device
  auto sizeBytes = random_trash.size() * sizeof(int);
  auto d_buffer = runtime_->mallocDevice(dev, sizeBytes);

  // copy from host to device and from device to result buffer host; check they are equal
  runtime_->memcpyHostToDevice(stream, reinterpret_cast<std::byte*>(random_trash.data()), d_buffer, sizeBytes);
  auto result = std::vector<int>(static_cast<unsigned long>(numElems));
  ASSERT_NE(random_trash, result);

  runtime_->memcpyDeviceToHost(stream, d_buffer, reinterpret_cast<std::byte*>(result.data()), sizeBytes);
  runtime_->waitForStream(stream);

  ASSERT_EQ(random_trash, result);
}

TEST_F(TestMemcpy, 4GbMemcpy) {
  using ValueType = uint32_t;
  std::mt19937 gen(std::random_device{}());

  auto dev = devices_[0];
  auto stream = runtime_->createStream(dev);
  auto desiredSize = 1ULL << 32;

  struct RandomData {
    ValueType value;
    ValueType position;
  };
  auto numValues = 500U;
  std::vector<RandomData> rd;
  // alloc memory in device
  auto d_buffer = runtime_->mallocDevice(dev, desiredSize);
  std::vector<ValueType> h_buffer(desiredSize / sizeof(ValueType));
  std::uniform_int_distribution<ValueType> dis(0, static_cast<ValueType>(h_buffer.size()-1));

  for (auto i = 0U; i < numValues; ++i) {
    rd.emplace_back(RandomData{dis(gen), dis(gen)});
  }
  for (auto v : rd) {
    h_buffer[v.position] = v.value;
  }

  // copy from host to device and from device to result buffer host; check they are equal
  runtime_->memcpyHostToDevice(stream, reinterpret_cast<std::byte*>(h_buffer.data()), d_buffer, desiredSize);

  //wait for stream to put 0s in the previously filled data before copying from device
  runtime_->waitForStream(stream);

  for (auto v: rd) {
    h_buffer[v.position] = 0U;
  }

  runtime_->memcpyDeviceToHost(stream, d_buffer, reinterpret_cast<std::byte*>(h_buffer.data()), desiredSize);
  runtime_->waitForStream(stream);

  for (auto v: rd) {
    EXPECT_EQ(h_buffer[v.position], v.value);
  }
}

} // namespace

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
