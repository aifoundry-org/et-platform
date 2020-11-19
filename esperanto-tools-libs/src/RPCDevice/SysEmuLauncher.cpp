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

#include "esperanto/runtime/Support/Logging.h"

#include <fcntl.h>
#include <fmt/format.h>
#include <glog/logging.h>
#include <limits>
#include <memory>
#include <stddef.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

namespace et_runtime {
namespace device {

SysEmuLauncher::SysEmuLauncher(
  const std::string &executable,
  const std::string &run_dir,
  const std::string &api_connection,
  uint64_t max_cycles,
  uint64_t shires_mask,
  const std::string &pu_uart0_tx_file,
  const std::string &pu_uart1_tx_file,
  const std::string &spio_uart0_tx_file,
  const std::string &spio_uart1_tx_file,
  const std::vector<std::string> &additional_options)
    : sysemu_run_(run_dir), device_alive_(false) {
  execute_args_ = {
      executable,             // Path to SysEMU's executable
      "-api_comm", api_connection,
      "-sim_api",
      "-max_cycles", std::to_string(max_cycles),
      "-minions", "FFFFFFFF", // All minions enabled
      "-shires", fmt::format("0x{:X}", shires_mask),
      "-mins_dis",            // Disable minions by default as booting is done through Sim API
      "-sp_dis",              // In case we are doing a SP boot, also start with SP not running
  };

  if (!pu_uart0_tx_file.empty()) {
    execute_args_.push_back("-pu_uart0_tx_file");
    execute_args_.push_back(pu_uart0_tx_file);
  } else {
    execute_args_.push_back("-pu_uart0_tx_file");
    execute_args_.push_back(sysemu_run_ + "/pu_uart0_tx.log");
  }

  if (!pu_uart1_tx_file.empty()) {
    execute_args_.push_back("-pu_uart1_tx_file");
    execute_args_.push_back(pu_uart1_tx_file);
  } else {
    execute_args_.push_back("-pu_uart1_tx_file");
    execute_args_.push_back(sysemu_run_ + "/pu_uart1_tx.log");
  }

  if (!spio_uart0_tx_file.empty()) {
    execute_args_.push_back("-spio_uart0_tx_file");
    execute_args_.push_back(spio_uart0_tx_file);
  } else {
    execute_args_.push_back("-spio_uart0_tx_file");
    execute_args_.push_back(sysemu_run_ + "/spio_uart0_tx.log");
  }

  if (!spio_uart1_tx_file.empty()) {
    execute_args_.push_back("-spio_uart1_tx_file");
    execute_args_.push_back(spio_uart1_tx_file);
  } else {
    execute_args_.push_back("-spio_uart1_tx_file");
    execute_args_.push_back(sysemu_run_ + "/spio_uart1_tx.log");
  }

  // Additional SysEMU options
  execute_args_.insert(execute_args_.end(), additional_options.begin(), additional_options.end());
}

SysEmuLauncher::~SysEmuLauncher() {}

/* Process */
void SysEmuLauncher::createProcess(const char *path,
                                   const std::vector<std::string> &argv,
                                   pid_t *pid) {

  // Prepare C-style argv (wrapped however by std::unique_ptr) with last additional NULL
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
    const char *envp[] = {// "GRPC_TRACE=api",
                          // "GRPC_VERBOSITY=DEBUG",
                          NULL};

    // tee sysemu to a log file and redirect there the stdout and stderr
    auto tee_log_file = fmt::format("tee {}/sysemu.log", sysemu_run_);
    auto tee = popen(tee_log_file.c_str(), "w");
    assert(tee != nullptr);

    auto pipe_fd = fileno(tee);
    assert(pipe_fd > 0);

    auto res = dup2(pipe_fd, STDOUT_FILENO);
    assert(res > 0);
    res = dup2(pipe_fd, STDERR_FILENO);
    assert(res > 0);

    execvpe(path, const_cast<char *const *>(c_argv.get()), const_cast<char *const *>(envp));
    int errno_val = errno;

    write(error_report_pipe_fd[1], &errno_val, sizeof(int));
    res = pclose(tee);
    assert(res == 0);

    exit(EXIT_FAILURE);
  } else {
    // parent
    close(error_report_pipe_fd[1]);

    int exec_errno = 0;
    if (TEMP_FAILURE_RETRY(read(error_report_pipe_fd[0], &exec_errno, sizeof(int))) != 0) {
      fprintf(stderr, "Failed to start sys-emu: %s (%s)\n", path, strerror(exec_errno));
      exit(EXIT_FAILURE);
    }
    close(error_report_pipe_fd[0]);
  }
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
  // on success we get the pid we waited on
  assert(ret_value == pid);
  if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0) {
    return true;
  }

  RTERROR << "SysEmu terminated abnormally";
  abort();
  return false;
}

} // namespace device

} // namespace et_runtime
