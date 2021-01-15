//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_COMMON_ABSL_H
#define ET_RUNTIME_COMMON_ABSL_H

#include <absl/flags/flag.h>

// TODO SW-4821: flags declaration is needed since we have two implementations
// in place which uses these flags i.e. TargetSysEmu.cc and SysEmuTarget_MM.cc
// TargetSysEmu.cc gives the definition of these flags and SysEmuTarget_MM.cc
// uses that same definition. Remove this declaration with mailbox
// implementation
ABSL_DECLARE_FLAG(std::string, sysemu_path);
ABSL_DECLARE_FLAG(std::string, sysemu_run_dir);
ABSL_DECLARE_FLAG(std::string, sysemu_socket_dir);
ABSL_DECLARE_FLAG(uint64_t, sysemu_max_cycles);
ABSL_DECLARE_FLAG(uint64_t, sysemu_shires_mask);
ABSL_DECLARE_FLAG(bool, sysemu_boot_sp);
ABSL_DECLARE_FLAG(std::string, sysemu_pu_uart0_tx_file);
ABSL_DECLARE_FLAG(std::string, sysemu_pu_uart1_tx_file);
ABSL_DECLARE_FLAG(std::string, sysemu_spio_uart0_tx_file);
ABSL_DECLARE_FLAG(std::string, sysemu_spio_uart1_tx_file);
ABSL_DECLARE_FLAG(std::string, sysemu_params);
ABSL_DECLARE_FLAG(std::string, bootrom_trampoline_to_bl2_elf);
ABSL_DECLARE_FLAG(std::string, bl2_elf);

#endif // ET_RUNTIME_COMMON_ABSL_H
