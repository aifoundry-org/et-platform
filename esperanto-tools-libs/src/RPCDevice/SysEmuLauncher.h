//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef DEVICE_SYS_EMU_LAUNCHER_H
#define DEVICE_SYS_EMU_LAUNCHER_H

#include <string>
#include <vector>

namespace et_runtime {
namespace device {

///
/// @brief Helper class responsible for launching SysEmu.
///
///
class SysEmuLauncher {

public:
  SysEmuLauncher(
      const std::string &executable,
      const std::string &run_dir,
      const std::string &api_connection,
      uint64_t max_cycles,
      uint64_t minion_shires_mask,
      const std::string &pu_uart0_tx_file,
      const std::string &pu_uart1_tx_file,
      const std::string &spio_uart0_tx_file,
      const std::string &spio_uart1_tx_file,
      const std::vector<std::string> &preload_elfs,
      const std::vector<std::string> &additional_options);
  ~SysEmuLauncher();

  bool launchSimulator();

private:
  std::string sysemu_run_; ///< Path to sysemu runfolder
  std::vector<std::string> execute_args_;  ///< Arguments we are going to use to instantiate sysemu
  bool device_alive_; ///< True iff the simulator has been launched

  void createProcess(const char *path, const std::vector<std::string> &argv,
                     pid_t *pid);
};

} // namespace device
} // namespace et_runtime

#endif // DEVICE_SYS_EMU_LAUNCHER_H
