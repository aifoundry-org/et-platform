#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <experimental/filesystem>

#include "deviceLayer/IDeviceLayer.h"
#include "Autogen.h"

/*
 * This test case is used to launch sysemu for TF/FIFO usecase
 */

namespace fs = std::experimental::filesystem;

namespace {
  constexpr uint64_t kSysEmuMaxCycles = std::numeric_limits<uint64_t>::max();
  constexpr uint64_t kSysEmuMinionShiresMask = 0x1FFFFFFFFu;
} // namespace

DEFINE_string(sysemu_params, "", "Extra parameters to pass to SysEMU");

TEST(TestLaunchSysemu, LaunchSuccess) 
{
    std::unique_ptr<dev::IDeviceLayer> devicLayerInst;
    emu::SysEmuOptions sysEmuOptions;
    std::istringstream iss{FLAGS_sysemu_params};

    sysEmuOptions.bootromTrampolineToBL2ElfPath = BOOTROM_TRAMPOLINE_TO_BL2_ELF;
    sysEmuOptions.spBL2ElfPath = BL2_ELF;
    sysEmuOptions.machineMinionElfPath = MACHINE_MINION_ELF;
    sysEmuOptions.masterMinionElfPath = MASTER_MINION_ELF;
    sysEmuOptions.workerMinionElfPath = WORKER_MINION_ELF;
    sysEmuOptions.executablePath = std::string(SYSEMU_INSTALL_DIR) + "sys_emu";
    sysEmuOptions.runDir = fs::current_path();
    sysEmuOptions.maxCycles = kSysEmuMaxCycles;
    sysEmuOptions.minionShiresMask = kSysEmuMinionShiresMask;
    sysEmuOptions.puUart0Path = sysEmuOptions.runDir + "/pu_uart0_tx.log";
    sysEmuOptions.puUart1Path = sysEmuOptions.runDir + "/pu_uart1_tx.log";

    // Route SP UART0 TX and RX traffic to named unix fifos used by TF
    sysEmuOptions.spUart0FifoInPath = "sp_uart0_rx"; 
    sysEmuOptions.spUart0FifoOutPath = "sp_uart0_tx";

    sysEmuOptions.startGdb = false;
    sysEmuOptions.additionalOptions =
      std::vector<std::string>{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};

    // Launch Sysemu through IDevice Abstraction
    devicLayerInst = dev::IDeviceLayer::createSysEmuDeviceLayer(sysEmuOptions);

    ASSERT_NE(devicLayerInst, nullptr);
}
