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
#include "TestDevOpsApiDmaCmds.h"
#include <experimental/filesystem>

/*
 * Test Labels: SYSEMU, OPS, FUNCTIONAL, SYSTEM
 */

namespace fs = std::experimental::filesystem;

DEFINE_string(sysemu_params, "", "Extra parameters to pass to SysEMU");

namespace {
constexpr uint64_t kSysEmuMaxCycles = std::numeric_limits<uint64_t>::max();
constexpr uint64_t kSysEmuMinionShiresMask = 0x1FFFFFFFFu;
} // namespace

class TestDevOpsApiFuncDmaCmdsSysEmu : public TestDevOpsApiDmaCmds {
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

TEST_F(TestDevOpsApiFuncDmaCmdsSysEmu, dataRWCmd_PositiveTesting_3_1) {
  dataRWCmd_PositiveTesting_3_1();
}

TEST_F(TestDevOpsApiFuncDmaCmdsSysEmu, dataRWCmdWithBasicCmds_3_4) {
  dataRWCmdWithBasicCmds_3_4();
}

TEST_F(TestDevOpsApiFuncDmaCmdsSysEmu, dataRWCmdMixed_3_5) {
  dataRWCmdMixed_3_5();
}

TEST_F(TestDevOpsApiFuncDmaCmdsSysEmu, dataRWCmdMixedWithVarSize_3_6) {
  dataRWCmdMixedWithVarSize_3_6();
}

TEST_F(TestDevOpsApiFuncDmaCmdsSysEmu, dataRWCmdAllChannels_3_7) {
  dataRWCmdAllChannels_3_7();
}

TEST_F(TestDevOpsApiFuncDmaCmdsSysEmu, dataRWCmdWithBarrier_PositiveTesting_3_10) {
  dataRWCmdWithBarrier_PositiveTesting_3_10();
}

TEST_F(TestDevOpsApiFuncDmaCmdsSysEmu, dataRWListCmd_PositiveTesting_3_11) {
  dataRWListCmd_PositiveTesting_3_11();
}

TEST_F(TestDevOpsApiFuncDmaCmdsSysEmu, dataRWListCmd_NegativeTesting_3_12) {
  dataRWListCmd_NegativeTesting_3_12();
}

/*
 * This test fixture executes all tests under same deviceLayer instantiation.
 * It helps validate cumulative effect of tests in SysEMU.
 */
TEST_F(TestDevOpsApiFuncDmaCmdsSysEmu, allTestsConsecutively) {
  dataRWCmd_PositiveTesting_3_1();
  dataRWCmdWithBasicCmds_3_4();
  dataRWCmdMixed_3_5();
  dataRWCmdMixedWithVarSize_3_6();
  dataRWCmdAllChannels_3_7();
  dataRWCmdWithBarrier_PositiveTesting_3_10();
  dataRWListCmd_PositiveTesting_3_11();
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
