//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------
#include "RuntimeImp.h"
#include "runtime/Types.h"
#include <common/Constants.h>
#include <device-layer/IDeviceLayer.h>
#include <device-layer/IDeviceLayerFake.h>
#include <experimental/filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logger.h>
#include <hostUtils/threadPool/ThreadPool.h>
#include <random>
#include <runtime/IRuntime.h>
#include <sw-sysemu/SysEmuOptions.h>

inline auto getDefaultOptions() {
  constexpr uint64_t kSysEmuMaxCycles = std::numeric_limits<uint64_t>::max();
  constexpr uint64_t kSysEmuMinionShiresMask = 0x1FFFFFFFFu;

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
  return sysEmuOptions;
}

inline std::vector<std::byte> readFile(const std::string& path) {
  auto file = std::ifstream(path, std::ios_base::binary);
  EXPECT_TRUE(file.is_open());
  if (!file.is_open()) {
    return {};
  }
  auto iniF = file.tellg();
  file.seekg(0, std::ios::end);
  auto endF = file.tellg();
  auto size = endF - iniF;
  file.seekg(0, std::ios::beg);

  std::vector<std::byte> fileContent(static_cast<uint32_t>(size));
  file.read(reinterpret_cast<char*>(fileContent.data()), size);
  return fileContent;
}

class Fixture : public testing::Test {
public:
  enum class Mode { PCIE, SYSEMU, FAKE };

  void SetUp() override {
    switch (sMode) {
    case Mode::PCIE:
      RT_LOG(INFO) << "Running tests with PCIE deviceLayer";
      deviceLayer_ = dev::IDeviceLayer::createPcieDeviceLayer();
      break;
    case Mode::SYSEMU:
      RT_LOG(INFO) << "Running tests with SYSEMU deviceLayer";
      deviceLayer_ = dev::IDeviceLayer::createSysEmuDeviceLayer(getDefaultOptions(), sNumDevices);
      break;
    case Mode::FAKE:
      RT_LOG(INFO) << "Running tests with FAKE deviceLayer";
      deviceLayer_ = std::make_unique<dev::IDeviceLayerFake>();
    }
    runtime_ = rt::IRuntime::create(deviceLayer_.get());
    devices_ = runtime_->getDevices();
    auto imp = static_cast<rt::RuntimeImp*>(runtime_.get());

    for (auto i = 0U; i < static_cast<uint32_t>(deviceLayer_->getDevicesCount()); ++i) {
      imp->setMemoryManagerDebugMode(devices_[i], true);
      defaultStreams_.emplace_back(runtime_->createStream(devices_[i]));
    }

    runtime_->setOnStreamErrorsCallback([](auto, const auto&) { FAIL(); });
    tp_ = std::make_unique<threadPool::ThreadPool>(1, true);
  }

  void TearDown() override {
    for (auto s : defaultStreams_) {
      runtime_->waitForStream(s);
      runtime_->destroyStream(s);
    }
    tp_.reset();
    runtime_.reset();
    defaultStreams_.clear();
    devices_.clear();
    deviceLayer_.reset();
  }

  rt::KernelId loadKernel(const std::string& kernel_name, uint32_t deviceIdx = 0) {
    auto kernelContent = readFile(std::string{KERNELS_DIR} + "/" + kernel_name);
    EXPECT_FALSE(kernelContent.empty());
    EXPECT_TRUE(devices_.size() > deviceIdx);
    auto tempStream = runtime_->createStream(rt::DeviceId(deviceIdx));
    auto res = runtime_->loadCode(tempStream, kernelContent.data(), kernelContent.size());
    tp_->pushTask([res, this, kernelContent = std::move(kernelContent)] { runtime_->waitForEvent(res.event_); });
    return res.kernel_;
  }

  inline static Mode sMode = Mode::SYSEMU;
  inline static uint8_t sNumDevices = 1;

protected:
  logging::LoggerDefault loggerDefault_;
  std::unique_ptr<dev::IDeviceLayer> deviceLayer_;
  rt::RuntimePtr runtime_;
  std::vector<rt::DeviceId> devices_;
  std::vector<rt::StreamId> defaultStreams_;
  std::unique_ptr<threadPool::ThreadPool> tp_;
};

template <typename TContainer> void randomize(TContainer& container, int init, int end) {
  std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution dis(init, end);
  for (auto& v : container) {
    v = static_cast<typename TContainer::value_type>(dis(gen));
  }
}

void stressMemThreadFunc(rt::IRuntime* runtime, uint32_t streams, uint32_t transactions, size_t bytes,
                         bool check_results, size_t deviceId) {
  ASSERT_LT(deviceId, runtime->getDevices().size());
  auto dev = runtime->getDevices()[deviceId];
  std::vector<rt::StreamId> streams_(streams);
  std::vector<std::vector<std::byte>> host_src(transactions);
  std::vector<std::vector<std::byte>> host_dst(transactions);
  std::vector<std::byte*> dev_mem(transactions);
  for (auto j = 0U; j < streams; ++j) {
    streams_[j] = runtime->createStream(dev);
    for (auto k = 0U; k < transactions / streams; ++k) {
      auto idx = k + j * transactions / streams;
      host_src[idx] = std::vector<std::byte>(bytes);
      host_dst[idx] = std::vector<std::byte>(bytes);
      randomize(host_src[idx], 0, 256);
      dev_mem[idx] = runtime->mallocDevice(dev, bytes);
      runtime->memcpyHostToDevice(streams_[j], host_src[idx].data(), dev_mem[idx], bytes);
      runtime->memcpyDeviceToHost(streams_[j], dev_mem[idx], host_dst[idx].data(), bytes);
    }
  }
  for (auto j = 0U; j < streams; ++j) {
    runtime->waitForStream(streams_[j]);
    for (auto k = 0U; k < transactions / streams; ++k) {
      auto idx = k + j * transactions / streams;
      runtime->freeDevice(dev, dev_mem[idx]);
      if (check_results) {
        ASSERT_THAT(host_dst[idx], testing::ElementsAreArray(host_src[idx]));
      }
    }
    runtime->destroyStream(streams_[j]);
  }
}

inline void run_stress_mem(rt::IRuntime* runtime, size_t bytes, uint32_t transactions, uint32_t streams,
                           uint32_t threads, bool check_results = true, size_t deviceId = 0) {
  std::vector<std::thread> threads_;
  using namespace testing;

  for (auto i = 0U; i < threads; ++i) {
    threads_.emplace_back(
      std::bind(stressMemThreadFunc, runtime, streams, transactions, bytes, check_results, deviceId));
  }
  for (auto& t : threads_) {
    t.join();
  }
}

inline bool IsPcie(int argc, char* argv[]) {
  for (auto i = 1; i < argc; ++i) {
    if (std::string{argv[i]} == "--mode=pcie") {
      return true;
    }
  }
  return false;
}