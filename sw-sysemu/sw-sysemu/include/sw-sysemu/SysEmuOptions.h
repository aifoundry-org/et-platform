//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#pragma once
#include <string>
#include <vector>
namespace emu {
/// \brief SysemuOptions are parameters to customize sysemu instantiation
struct SysEmuOptions {
  /// \brief Bootrom trampoline to BL2 elf path to load into sysemu
  std::string bootromTrampolineToBL2ElfPath;
  /// \brief Service Processor BL2 elf path to load into sysemu
  std::string spBL2ElfPath;
  /// \brief machine minion elf path to load into sysemu
  std::string machineMinionElfPath;
  /// \brief master minion elf path to load into sysemu
  std::string masterMinionElfPath;
  /// \brief worker minion elf path to load into sysemu
  std::string workerMinionElfPath;
  /// \brief absolute path to sysemu executable
  std::string executablePath;
  /// \brief working directory for sysemu
  std::string runDir;
  /// \brief maximum cycles to run before finishing simulation
  uint64_t maxCycles;
  /// \brief sysemu Minion Shire mask (enabled shires)
  uint64_t minionShiresMask;
  /// \brief SysEmu PU UART0 TX log file
  std::string puUart0Path;
  /// \brief SysEmu PU UART1 TX log file
  std::string puUart1Path;
  /// \brief SysEmu SPIO UART0 TX log file
  std::string spUart0Path;
  /// \brief SysEmu SPIO UART1 TX log file
  std::string spUart1Path;
  /// \brief SysEmu PU UART1 TX
  std::string puUart1FifoOutPath; // To support for bring-up test framework
  /// \brief SysEmu PU UART1 RX
  std::string puUart1FifoInPath; // To support bring-up test framework
  /// \brief SysEmu SPIO UART0 TX
  std::string spUart0FifoOutPath; // To support for bring-up test framework
  /// \brief SysEmu SPIO UART0 RX
  std::string spUart0FifoInPath; // To support bring-up test framework
  /// \brief SysEmu SPIO UART1 TX
  std::string spUart1FifoOutPath; // To support for bring-up test framework
  /// \brief SysEmu SPIO UART1 RX
  std::string spUart1FifoInPath; // To support bring-up test framework
  /// \brief Start GDB stub
  bool startGdb;
  /// \brief Log file name
  std::string logFile = "sysemu.log";
  /// \brief Enable coherence memory checks in sysemu
  bool memcheck = true;
  /// \brief Enable L2scp checks
  bool l2ScpCheck = true;
  /// \brief Enable L1scp checks 
  bool l1ScpCheck = true;
  /// \brief Enable FLB checks 
  bool flbCheck = true;
  /// \brief Defaults memory to this value
  uint32_t mem_reset32 = 0xDEADBEEF;
  /// \brief Hyperparameters to pass to SysEmu, might override default values
  std::vector<std::string> additionalOptions;
};
} // namespace emu
