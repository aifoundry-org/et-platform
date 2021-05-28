#include <common/Constants.h>
#include <hostUtils/logging/Logger.h>
#include <device-layer/IDeviceLayer.h>
#include <experimental/filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <runtime/IRuntime.h>
#include <sw-sysemu/SysEmuOptions.h>

inline std::vector<std::byte> readFile(const std::string &path) {
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

  std::vector<std::byte> fileContent(size);
  file.read(reinterpret_cast<char *>(fileContent.data()), size);
  return fileContent;
}


/* derive this class with mocked / non-mocked DeviceLayer. Intented to be instantiated in google test parameterized tests (TEST_P) */
class Fixture : public testing::Test { 
public:
  void init(std::unique_ptr<dev::IDeviceLayer> deviceLayer) {
      deviceLayer_ = std::move(deviceLayer);
      runtime_ = rt::IRuntime::create(deviceLayer_.get());
      devices_ = runtime_->getDevices();
  }

  rt::KernelId loadKernel(const std::string& kernel_name, int deviceIdx = 0) {
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

