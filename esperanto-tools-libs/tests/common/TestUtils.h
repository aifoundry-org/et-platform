#include "RuntimeImp.h"
#include <common/Constants.h>
#include <device-layer/IDeviceLayer.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logger.h>
#include <runtime/IRuntime.h>
#include <sw-sysemu/SysEmuOptions.h>

#include <experimental/filesystem>
#include <fstream>
#include <random>

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

/* derive this class with mocked / non-mocked DeviceLayer. Intented to be instantiated in google test parameterized
 * tests (TEST_P) */
class Fixture : public testing::Test {
public:
  void init(std::unique_ptr<dev::IDeviceLayer> deviceLayer) {
    deviceLayer_ = std::move(deviceLayer);
    runtime_ = rt::IRuntime::create(deviceLayer_.get());
    devices_ = runtime_->getDevices();
    auto imp = static_cast<rt::RuntimeImp*>(runtime_.get());
    imp->setMemoryManagerDebugMode(devices_[0], true);
  }

  rt::KernelId loadKernel(const std::string& kernel_name, uint32_t deviceIdx = 0) {
    auto kernelContent = readFile(std::string{KERNELS_DIR} + "/" + kernel_name);
    EXPECT_FALSE(kernelContent.empty());
    EXPECT_TRUE(devices_.size() > deviceIdx);
    return runtime_->loadCode(devices_[deviceIdx], kernelContent.data(), kernelContent.size());
  }

protected:
  logging::LoggerDefault loggerDefault_;
  std::unique_ptr<dev::IDeviceLayer> deviceLayer_;
  rt::RuntimePtr runtime_;
  std::vector<rt::DeviceId> devices_;
};

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
template <typename TContainer> void randomize(TContainer& container, int init, int end) {
  std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution dis(init, end);
  for (auto& v : container) {
    v = static_cast<typename TContainer::value_type>(dis(gen));
  }
}

void stressMemThreadFunc(rt::IRuntime* runtime, uint32_t streams, uint32_t transactions, size_t bytes, bool check_results) {
  auto dev = runtime->getDevices()[0];
  std::vector<rt::StreamId> streams_(streams);
  std::vector<std::vector<std::byte>> host_src(transactions);
  std::vector<std::vector<std::byte>> host_dst(transactions);
  std::vector<void*> dev_mem(transactions);
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
                           uint32_t threads, bool check_results = true) {
  std::vector<std::thread> threads_;
  using namespace testing;

  for (auto i = 0U; i < threads; ++i) {
    threads_.emplace_back(std::bind(stressMemThreadFunc, runtime, streams, transactions, bytes, check_results));
  }
  for (auto& t : threads_) {
    t.join();
  }
}