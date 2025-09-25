//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#pragma once
#include <cstdint>
#include <string>
#include <vector>

#pragma GCC visibility push(default)

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
  /// \brief Enable TensorStore checks
  bool tstoreCheck = true;
  /// \brief Defaults memory to this value
  uint32_t mem_reset32 = 0xDEADBEEF;
  /// \brief Hyperparameters to pass to SysEmu, might override default values
  std::vector<std::string> additionalOptions;
};
} // namespace emu

#pragma GCC visibility pop
