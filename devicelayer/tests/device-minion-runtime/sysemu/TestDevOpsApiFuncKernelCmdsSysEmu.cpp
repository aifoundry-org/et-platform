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
#include "TestDevOpsApiKernelCmds.h"
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

class TestDevOpsApiFuncKernelCmdsSysEmu : public TestDevOpsApiKernelCmds {
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

TEST_F(TestDevOpsApiFuncKernelCmdsSysEmu, launchAddVectorKernel_PositiveTesting_4_1) {
  launchAddVectorKernel_PositiveTesting_4_1(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsSysEmu, launchAddVectorKernel_VariableShireMasks) {
  launchAddVectorKernel_PositiveTesting_4_1((0x1 & kSysEmuMinionShiresMask));         /* Shire 0 (if available) */
  launchAddVectorKernel_PositiveTesting_4_1((0x3 & kSysEmuMinionShiresMask));         /* Shire 0-1 (if available) */
  launchAddVectorKernel_PositiveTesting_4_1((0x7 & kSysEmuMinionShiresMask));         /* Shire 0-2 (if available) */
  launchAddVectorKernel_PositiveTesting_4_1((0xF & kSysEmuMinionShiresMask));         /* Shire 0-4 (if available) */
  launchAddVectorKernel_PositiveTesting_4_1((0xFF & kSysEmuMinionShiresMask));        /* Shire 0-8 (if available) */
  launchAddVectorKernel_PositiveTesting_4_1((0xFFFF & kSysEmuMinionShiresMask));      /* Shire 0-16 (if available) */
  launchAddVectorKernel_PositiveTesting_4_1((0x1FFFFFFFF & kSysEmuMinionShiresMask)); /* Shire 0-32 (if available) */
}

TEST_F(TestDevOpsApiFuncKernelCmdsSysEmu, launchUberKernel_PositiveTesting_4_4) {
  launchUberKernel_PositiveTesting_4_4(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsSysEmu, launchEmptyKernel_PositiveTesting_4_5) {
  launchEmptyKernel_PositiveTesting_4_5(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsSysEmu, launchExceptionKernel_NegativeTesting_4_6) {
  launchExceptionKernel_NegativeTesting_4_6(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsSysEmu, abortHangKernel_PositiveTesting_4_10) {
  launchHangKernel(kSysEmuMinionShiresMask, true);
}

/*
 * This test fixture executes all tests under same deviceLayer instantiation.
 * It helps validate cumulative effect of tests in SysEMU.
 */
TEST_F(TestDevOpsApiFuncKernelCmdsSysEmu, allTestsConsecutively) {
  launchAddVectorKernel_PositiveTesting_4_1(kSysEmuMinionShiresMask);
  launchUberKernel_PositiveTesting_4_4(kSysEmuMinionShiresMask);
  launchEmptyKernel_PositiveTesting_4_5(kSysEmuMinionShiresMask);
  launchExceptionKernel_NegativeTesting_4_6(kSysEmuMinionShiresMask);
  launchHangKernel(kSysEmuMinionShiresMask, true);
}

TEST_F(TestDevOpsApiFuncKernelCmdsSysEmu, kernelAbortCmd_InvalidTagIdNegativeTesting_6_2) {
  kernelAbortCmd_InvalidTagIdNegativeTesting_6_2();
}

TEST_F(TestDevOpsApiFuncKernelCmdsSysEmu, launchAddVectorKernel_CM_UModeTesting_6_1) {
  // provide the new kernel elf that uses the cm-umode lib
  launchAddVectorKernel_PositiveTesting_4_1(kSysEmuMinionShiresMask, "cm_umode_test.elf");
}

/**********************************************************
*                                                         *
*          Kernel DMA LIST Tests                          *
*                                                         *
**********************************************************/

TEST_F(TestDevOpsApiFuncKernelCmdsSysEmu, launchAddVectorKernelDMAList_PositiveTesting_4_11) {
  launchAddVectorKernelListCmd(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsSysEmu, launchAddVectorKernelDMAList_VariableShireMasks) {
  launchAddVectorKernelListCmd((0x1 & kSysEmuMinionShiresMask));         /* Shire 0 (if available) */
  launchAddVectorKernelListCmd((0x3 & kSysEmuMinionShiresMask));         /* Shire 0-1 (if available) */
  launchAddVectorKernelListCmd((0x7 & kSysEmuMinionShiresMask));         /* Shire 0-2 (if available) */
  launchAddVectorKernelListCmd((0xF & kSysEmuMinionShiresMask));         /* Shire 0-4 (if available) */
  launchAddVectorKernelListCmd((0xFF & kSysEmuMinionShiresMask));        /* Shire 0-8 (if available) */
  launchAddVectorKernelListCmd((0xFFFF & kSysEmuMinionShiresMask));      /* Shire 0-16 (if available) */
  launchAddVectorKernelListCmd((0x1FFFFFFFF & kSysEmuMinionShiresMask)); /* Shire 0-32 (if available) */
}

TEST_F(TestDevOpsApiFuncKernelCmdsSysEmu, launchUberKernelDMAList_PositiveTesting_4_14) {
  launchUberKernelListCmd(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsSysEmu, launchEmptyKernelDMAList_PositiveTesting_4_15) {
  launchEmptyKernelListCmd(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsSysEmu, launchExceptionKernelDMAList_NegativeTesting_4_16) {
  launchExceptionKernelListCmd(kSysEmuMinionShiresMask);
}

TEST_F(TestDevOpsApiFuncKernelCmdsSysEmu, abortHangKernelDMAList_PositiveTesting_4_17) {
  launchHangKernelListCmd(kSysEmuMinionShiresMask, true);
}

/*
 * This test fixture executes all tests under same deviceLayer instantiation.
 * It helps validate cumulative effect of tests in SysEMU.
 */
TEST_F(TestDevOpsApiFuncKernelCmdsSysEmu, allTestsConsecutivelyKernelDMAList) {
  launchAddVectorKernelListCmd(kSysEmuMinionShiresMask);
  launchUberKernelListCmd(kSysEmuMinionShiresMask);
  launchEmptyKernelListCmd(kSysEmuMinionShiresMask);
  launchExceptionKernelListCmd(kSysEmuMinionShiresMask);
  launchHangKernelListCmd(kSysEmuMinionShiresMask, true);
}


TEST_F(TestDevOpsApiFuncKernelCmdsSysEmu, launchAddVectorKernelDMAList_CM_UModeTesting_6_11) {
  // provide the new kernel elf that uses the cm-umode lib
  launchAddVectorKernelListCmd(kSysEmuMinionShiresMask, KernelTypes::CMUMODE_KERNEL_TYPE);
}


int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
