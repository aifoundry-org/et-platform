//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "SysEmuLauncher.h"

#include "Core/CommandLineOptions.h"
#include "Device/TargetDeviceInfo.h"
#include "Support/Logging.h"

#include <fcntl.h>
#include <fmt/format.h>
#include <glog/logging.h>
#include <memory>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>

using namespace std;

ABSL_FLAG(bool, sysemu_log_enable, false, "Enable sysemu logging");
ABSL_FLAG(int64_t, sysemu_log_minion, -1, "Enable logging of minion X");
ABSL_FLAG(std::string, sysemu_pu_uart_tx_file, "", "Set sysemu PU UART TX log file");
ABSL_FLAG(std::string, sysemu_pu_uart1_tx_file, "", "Set sysemu PU UART1 TX log file");

namespace et_runtime {
namespace device {

SysEmuLauncher::SysEmuLauncher(
    const std::string &con, const std::vector<std::string> &additional_options)
    : connection_(con), device_alive_(false) {
  execute_args_ = {
      absl::GetFlag(FLAGS_sysemu_path), // Path to SysEMU
      "-minions",
      "FFFFFFFF",             // All minions enabled
      "-shires", "1FFFFFFFF", // All shires enabled
      "-master_min",          // Enable master shire
      std::string("-api_comm"), connection_,
      "-mins_dis", // Disable minions by default as booting is done through an
                   // exec commandi
      //"-l",//"-lm","0", // Enable logging of minion0
      "-max_cycles", "1000000000", // Limiting number of virtual simulation cycles.
                                // Increase if needed
  };
  execute_args_.insert(execute_args_.end(), additional_options.begin(),
                       additional_options.end());

  if (absl::GetFlag(FLAGS_fw_type).type == "device-fw") {
    execute_args_.push_back("-sim_api_async");
  }
  if (absl::GetFlag(FLAGS_sysemu_log_enable)) {
    execute_args_.push_back("-l");
  }
  if (auto mx = absl::GetFlag(FLAGS_sysemu_log_minion); mx > 0) {
    execute_args_.insert(execute_args_.end(),
                         {"-l", "-lm", fmt::format("{}", mx)});
  }
  if (auto file = absl::GetFlag(FLAGS_sysemu_pu_uart_tx_file); !file.empty()) {
    execute_args_.push_back("-pu_uart_tx_file");
    execute_args_.push_back(file);
  }
  if (auto file = absl::GetFlag(FLAGS_sysemu_pu_uart1_tx_file); !file.empty()) {
    execute_args_.push_back("-pu_uart1_tx_file");
    execute_args_.push_back(file);
  }
}

SysEmuLauncher::SysEmuLauncher(const std::string &con)
    : SysEmuLauncher(con, {}) {}

SysEmuLauncher::~SysEmuLauncher() {}

/* Process */
void SysEmuLauncher::createProcess(const char *path,
                                   const std::vector<std::string> &argv,
                                   pid_t *pid) {

  // prepare C-style argv (wrapped however by std::unique_ptr) with last
  // additional NULL
  std::unique_ptr<const char *[]> c_argv(new const char *[argv.size() + 1]);
  for (size_t i = 0; i < argv.size(); i++) {
    c_argv[i] = argv[i].c_str();
  }
  c_argv[argv.size()] = NULL;

  // pipe used to report error from execve call in child process
  int error_report_pipe_fd[2];
  pipe2(error_report_pipe_fd, O_CLOEXEC);

  *pid = fork();

  if (*pid == 0) {
    // child
    // kill when parent dies
    prctl(PR_SET_PDEATHSIG, SIGHUP);
    close(error_report_pipe_fd[0]);
    const char* envp[] = {
      // "GRPC_TRACE=api",
      // "GRPC_VERBOSITY=DEBUG",
      NULL
    };
    execvpe(path, const_cast<char *const *>(c_argv.get()), const_cast<char* const*>(envp)) ;
    int errno_val = errno;

    write(error_report_pipe_fd[1], &errno_val, sizeof(int));
    exit(EXIT_FAILURE);
  }

  // parent
  close(error_report_pipe_fd[1]);
  int exec_errno = 0;
  if (TEMP_FAILURE_RETRY(
          read(error_report_pipe_fd[0], &exec_errno, sizeof(int))) != 0) {
    fprintf(stderr, "Failed to start sys-emu: %s (%s)\n", path,
            strerror(exec_errno));
    exit(EXIT_FAILURE);
  }
  close(error_report_pipe_fd[0]);
}

bool SysEmuLauncher::launchSimulator() {
  std::string sys_emu_cmd = "";
  for (auto &i : execute_args_) {
    sys_emu_cmd += i + " ";
  }

  RTINFO << "SysEmu invocation: " << sys_emu_cmd;
  // Launch the Sysemu process
  pid_t pid;
  createProcess(execute_args_[0].c_str(), execute_args_, &pid);
  RTINFO << "SysEmu process started with pid: " << pid;

  // We are expecting that we have sent a shutdown command to the simulator
  // and the simulator terminates gracefully.
  // FIXME we are not checking the exit code this is sad
  RTINFO << "Wait for SysEmu process to terminate: " << pid;
  int wstatus = 0;
  auto ret_value = waitpid(pid, &wstatus, 0);
  if (ret_value < 0) {
    if (!WIFEXITED(wstatus)) {
      RTERROR << "SysEmu terminated abnormally";
      abort();
    }
  }
  return true;
}

} // namespace device

} // namespace et_runtime
