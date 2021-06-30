//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "common/Constants.h"
#include "device-layer/IDeviceLayer.h"
#include "runtime/IRuntime.h"
#include "sw-sysemu/SysEmuOptions.h"
#include "utils.h"

#include <hostUtils/logging/Logger.h>
#include <experimental/filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <ios>

namespace fs = std::experimental::filesystem;

namespace {
constexpr uint64_t kSysEmuMaxCycles = std::numeric_limits<uint64_t>::max();
constexpr uint64_t kSysEmuMinionShiresMask = 0x1FFFFFFFFu;

class TestCodeLoading : public ::testing::Test {
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
    auto elf_file = std::ifstream((fs::path(KERNELS_DIR) / fs::path("add_vector.elf")).string(), std::ios::in | std::ios::binary);
    ASSERT_TRUE(elf_file.is_open());
    elf_file.seekg(0, std::ios::end);
    auto size = elf_file.tellg();
    ASSERT_GT(size, 0);
    convolutionContent_.resize(static_cast<unsigned long>(size));
    elf_file.seekg(0);
    elf_file.read(reinterpret_cast<char*>(convolutionContent_.data()), size);
  }

  void TearDown() override {
    runtime_.reset();
  }

  std::unique_ptr<dev::IDeviceLayer> deviceLayer_;
  rt::RuntimePtr runtime_;
  std::vector<std::byte> convolutionContent_;
  std::vector<rt::DeviceId> devices_;
};

// Load and removal of a single kernel.
TEST_F(TestCodeLoading, LoadKernel) {

  rt::KernelId kernel;
  EXPECT_NO_THROW(kernel =
                    runtime_->loadCode(devices_.front(), convolutionContent_.data(), convolutionContent_.size()));
  EXPECT_NO_THROW(runtime_->unloadCode(kernel));
  // if we unload again the same kernel we should expect to throw an exception
  EXPECT_THROW(runtime_->unloadCode(kernel), rt::Exception);
}

TEST_F(TestCodeLoading, MultipleLoads) {
  std::vector<rt::KernelId> kernels;

  RT_LOG(INFO) << "Loading 100 kernels";
  for (int i = 0; i < 100; ++i) {
    EXPECT_NO_THROW(kernels.emplace_back(
      runtime_->loadCode(devices_.front(), convolutionContent_.data(), convolutionContent_.size())));
  }
  for (auto it = begin(kernels) + 1; it != end(kernels); ++it) {
    EXPECT_LT(static_cast<uint32_t>(*(it - 1)), static_cast<uint32_t>(*it));
  }
  RT_LOG(INFO) << "Unloading 100 kernels";
  for (auto kernel : kernels) {
    EXPECT_NO_THROW(runtime_->unloadCode(kernel));
  }
}

} // namespace

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  g3::log_levels::disable(DEBUG);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
