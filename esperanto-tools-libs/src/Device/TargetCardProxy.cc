//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------
#include "TargetCardProxy.h"

#include "Device/TargetDeviceInfo.h"
#include "Support/Logging.h"
#include "et-rpc.h"

#include "et_socket_addr.h"
#include "etrpc/et-card-proxy.h"

#include <csignal>
#include <fcntl.h>
#include <fmt/format.h>
#include <glog/logging.h>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <vector>

using namespace std;

namespace et_runtime {
namespace device {

CardProxyTarget::CardProxyTarget(const std::string &path) : DeviceTarget(path) {
  connection_ = fmt::format("/tmp/card_emu_socket_{}", getpid());
  execute_args_ = {
      SYSEMU_PATH, //
      "-minions",
      "FFFFFFFF",            // All minions enabled
      "-shires", "FFFFFFFF", // All shires enabled
      std::string("-api_comm"), connection_,
      "-mins_dis", // Disable minions by default as booting is done through an
                   // exec command
      // "-l","-lm","0" // Enable logging of minion0
      "-max_cycles", "800000", // Limiting number of virtual simulation cycles.
                               // Increase if needed
  };
  card_proxy_ = std::make_unique<CardProxy>(connection_);
}

CardProxyTarget::~CardProxyTarget() {}

/* Process */
static void createProcess(const char *path,
                          const std::vector<std::string> &argv, pid_t *pid) {

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
    close(error_report_pipe_fd[0]);
    execv(path, const_cast<char *const *>(c_argv.get()));
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

bool CardProxyTarget::launchSimulator() {
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
  waitpid(pid, NULL, 0);
  return true;
}

void CardProxyTarget::waitForConnection() {
  // HACK use initSocket directlry and start a server that SysEmu can connect
  // to This eliminates the card-proxy from the middle.
  RTINFO << "Waiting for SysEmu to connect";
  card_proxy_->card_emu().init();
  RTINFO << "SysEmu connected.";
  return;
}

bool CardProxyTarget::init() {

  if (device_alive_) {
    return false;
  }
  RTINFO << "Starting SysEmu Process";
  RTINFO << "SysEmu socket: " << path_;

  // Set the path that we will use for the socket.
  DLOG(INFO) << "Initializing " << name_ << " to socket path: " << connection_;
  etrtSetClientSocketPath(connection_);

  // Start thread that will wait for SysEMU to connect.
  thread connection_listener = thread([this]() { this->waitForConnection(); });

  // Launch the simulator
  simulator_status_ = async(
      launch::async, [this]() -> auto { return this->launchSimulator(); });

  // simulator_thread_ = thread([this]() { this->launchSimulator(); });

  // Wait until sysemu has connected to the socket
  connection_listener.join();
  device_alive_ = true;
  return true;
}

bool CardProxyTarget::deinit() {
  /// If the simulator is not alive any more return
  if (!this->alive()) {
    return false;
  }
  // If ti is alive then send a shut down command
  card_proxy_->card_emu().shutdown();
  // Wait until we have killed the simulator
  simulator_status_.wait();
  return true;
}

bool CardProxyTarget::getStatus() {
  assert(true);
  return false;
}

bool CardProxyTarget::getStaticConfiguration() {
  assert(true);
  return false;
}

bool CardProxyTarget::submitCommand() {
  assert(true);
  return false;
}

bool CardProxyTarget::registerResponseCallback() {
  assert(true);
  return false;
}

bool CardProxyTarget::registerDeviceEventCallback() {
  assert(true);
  return false;
}

bool CardProxyTarget::alive() {
  auto status = simulator_status_.wait_for(0ms);

  if (status == std::future_status::ready) {
    simulator_status_.wait();
    return false;
  }
  return true;
}

} // namespace device
} // namespace et_runtime
