//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Autogen.h"
#include "TestDevOpsApiBasicCmds.h"
#include <experimental/filesystem>

/*
 * Test Labels: SYSEMU, OPS, FUNCTIONAL, SYSTEM
 */

using namespace dev::dl_tests;
namespace fs = std::experimental::filesystem;

DEFINE_string(sysemu_params, "", "Extra parameters to pass to SysEMU");

namespace {
constexpr uint64_t kSysEmuMaxCycles = std::numeric_limits<uint64_t>::max();
constexpr uint64_t kSysEmuMinionShiresMask = 0x1FFFFFFFFu;
} // namespace

class TestDevOpsApiFuncBasicCmdsSysEmu : public TestDevOpsApiBasicCmds {
protected:
  void SetUp() override {
    execTimeout_ = std::chrono::seconds(FLAGS_exec_timeout);
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
    sysEmuOptions.spUart0Path = sysEmuOptions.runDir + "/spio_uart0_tx.log";
    sysEmuOptions.spUart1Path = sysEmuOptions.runDir + "/spio_uart1_tx.log";
    sysEmuOptions.startGdb = false;
    sysEmuOptions.additionalOptions =
      std::vector<std::string>{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};

    initTestHelperSysEmu(sysEmuOptions);
  }
};

TEST_F(TestDevOpsApiFuncBasicCmdsSysEmu, echoCmd_PositiveTest_2_1) {
  echoCmd_PositiveTest_2_1();
}

TEST_F(TestDevOpsApiFuncBasicCmdsSysEmu, devCompatCmd_PositiveTest_2_3) {
  devCompatCmd_PositiveTest_2_3();
}

TEST_F(TestDevOpsApiFuncBasicCmdsSysEmu, devFWCmd_PostiveTest_2_5) {
  devFWCmd_PostiveTest_2_5();
}

TEST_F(TestDevOpsApiFuncBasicCmdsSysEmu, devUnknownCmd_NegativeTest_2_7) {
  devUnknownCmd_NegativeTest_2_7();
}

/*
 * This test fixture executes all tests under same deviceLayer instantiation.
 * It helps validate cumulative effect of tests in SysEMU.
 */
TEST_F(TestDevOpsApiFuncBasicCmdsSysEmu, allTestsConsecutively) {
  echoCmd_PositiveTest_2_1();
  devCompatCmd_PositiveTest_2_3();
  devFWCmd_PostiveTest_2_5();
  devUnknownCmd_NegativeTest_2_7();
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
