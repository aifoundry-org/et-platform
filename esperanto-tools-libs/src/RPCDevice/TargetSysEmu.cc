//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TargetSysEmu.h"

#include "esperanto/simulator-api.grpc.pb.h"

#include "RPCDevice/TargetDeviceInfo.h"
#include "esperanto/runtime/Support/Logging.h"

#include <absl/flags/flag.h>
#include <cstdio>
#include <experimental/filesystem>
#include <fmt/format.h>
#include <limits>
#include <random>
#include <string>
#include <string_view>
#include <sys/un.h>
#include <thread>
namespace fs = std::experimental::filesystem;

using namespace std;
using namespace simulator_api;

ABSL_FLAG(std::string, sysemu_path, et_runtime::device::SYSEMU_PATH,
          "Path to sysemu");

ABSL_FLAG(std::string, sysemu_socket_dir, "",
          "Existing folder where sysemu sockets are going to be created, "
          "expected < 100 characters.");

// SysEMU run directory, there we will create the socket file and
// store any uart logs from sysemu
ABSL_FLAG(std::string, sysemu_run_dir, "", "Path to sysemu run folder");

namespace et_runtime {
namespace device {
TargetSysEmu::TargetSysEmu(int index) : RPCTarget(index, "") {
  // Find the current directory
  // Do not use the full path as the path we provide needs to fit in 107
  // characters
  auto cwd = fs::current_path();
  auto pid = getpid();
  auto opt_path = absl::GetFlag(FLAGS_sysemu_run_dir);
  // The runtime + sysemu could be costructed/destructed multiple times in the
  // lifetime of a process (e.g. google test) The PID is not sufficient for
  // separating the run folder
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<uint32_t> dist(
      0, std::numeric_limits<uint32_t>::max());
  auto rand = dist(mt);
  auto run_folder =
      !opt_path.empty()
          ? fs::path(opt_path)
          : cwd / fs::path(fmt::format("sysemu_run_{}_{}",
                                       static_cast<uint64_t>(pid), rand));

  RTINFO << "SysEmu Run Folder" << run_folder << "\n";
  if (fs::exists(run_folder)) {
    RTERROR << "Folder already exists " << run_folder << "\n";
    std::terminate();
  }
  fs::create_directories(run_folder);
  // Create a unique name for an abtract unix socket.
  // Using the "@" allows us to create the socket inside the currently running
  // directory We do not need to provide the full path to the "unix:@<SOCKET>"
  // URL to GRPC, but this URL still  has a 107 character length limit
  auto socket_name = fmt::format("sysemu_{}_{}.sock", pid, rand);
  // Check that the file path we provide can fit in the max length of a socket.
  // to be passed to GRPC
  sockaddr_un fsocket;

  RTINFO << "Abstract unit socket name : " + socket_name << "\n";

  // Use an abstract socket and not an actuall file
  // See
  auto socket_dir = absl::GetFlag(FLAGS_sysemu_socket_dir);
  if (socket_dir.size() > 0) {
    auto sock_path = fs::path(socket_dir) / socket_name;
    path_ = std::string("unix://") + sock_path.string();
  } else {
    path_ = std::string("unix:@") + socket_name;
  }
  assert(path_.size() < sizeof(fsocket.sun_path));
  sys_emu_ = make_unique<SysEmuLauncher>(run_folder.string(), path_,
                                         vector<string>({"-sim_api"}));
};

TargetSysEmu::~TargetSysEmu() {}

bool TargetSysEmu::init() {

  if (device_alive_) {
    return false;
  }

  auto init_grpc = RPCTarget::init();
  assert(init_grpc);

  RTINFO << "Starting SysEmu Process";
  RTINFO << "SysEmu socket: " << path_;

  // Set the path that we will use for the socket.
  DLOG(INFO) << "Initializing " << name_ << " to socket path: " << path_;

  // Start thread that will wait for SysEMU to connect.
  // thread connection_listener = thread([this]() { this->waitForConnection();
  // });

  // Launch the simulator
  simulator_status_ = async(launch::async, [this]() -> auto {
    return this->sys_emu_->launchSimulator();
  });

  // Wait until sysemu has connected to the socket
  // connection_listener.join();
  device_alive_ = true;

  return true;
}

bool TargetSysEmu::postFWLoadInit() {

#if ENABLE_DEVICE_FW
  // Boot device-fw
  boot(0x8000001000);

#endif // ENABLE_DEVICE_FW
  auto success = RPCTarget::postFWLoadInit();
  assert(success);
  return true;
}

bool TargetSysEmu::deinit() {
  /// If the simulator is not alive any more return
  if (!this->alive()) {
    return false;
  }
  // If it is alive then send a shut down command
  auto res = this->shutdown();
  assert(res);
  // Wait until we have killed the simulator
  simulator_status_.wait();
  return true;
}

bool TargetSysEmu::alive() {
  auto status = simulator_status_.wait_for(0ms);

  // check if the simulator has startedy
  if (status == std::future_status::ready) {
    simulator_status_.wait();
    return false;
  }
  return device_alive_;
}

bool TargetSysEmu::getStatus() {
  assert(true);
  return false;
}

DeviceInformation TargetSysEmu::getStaticConfiguration() {
  assert(true);
  return {};
}

bool TargetSysEmu::submitCommand() {
  assert(true);
  return false;
}

bool TargetSysEmu::registerResponseCallback() {
  assert(true);
  return false;
}

bool TargetSysEmu::registerDeviceEventCallback() {
  assert(true);
  return false;
}
} // namespace device
} // namespace et_runtime
