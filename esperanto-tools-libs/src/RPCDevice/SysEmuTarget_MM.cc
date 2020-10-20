//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "SysEmuTarget_MM.h"

#include "esperanto/simulator-api.grpc.pb.h"

#include "RPCDevice/TargetDeviceInfo.h"
#include "esperanto/runtime/Support/Logging.h"
#include "SysEmuAbslFlags.h"

#include <cstdio>
#include <experimental/filesystem>
#include <fmt/format.h>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/un.h>
#include <thread>
namespace fs = std::experimental::filesystem;

using namespace std;
using namespace simulator_api;

namespace et_runtime {
namespace device {
SysEmuTargetMM::SysEmuTargetMM(int index) : RPCTargetMM(index, "") {
  // Find the current directory
  // Do not use the full path as the path we provide needs to fit in 107 characters
  auto cwd = fs::current_path();
  auto pid = getpid();
  auto opt_path = absl::GetFlag(FLAGS_sysemu_run_dir);

  // The runtime + sysemu could be costructed/destructed multiple times in the lifetime of a
  // process (e.g. google test) The PID is not sufficient for separating the run folder
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<uint32_t> dist(0, std::numeric_limits<uint32_t>::max());
  auto rand = dist(mt);

  std::string run_folder;
  if (!opt_path.empty()) {
    run_folder = fs::path(opt_path);
  } else {
    run_folder = cwd / fs::path(fmt::format("sysemu_run_{}_{}", static_cast<uint64_t>(pid), rand));
  }

  RTINFO << "SysEmu Run Folder" << run_folder << "\n";
  if (fs::exists(run_folder)) {
    RTERROR << "Folder already exists " << run_folder << "\n";
    std::terminate();
  }
  fs::create_directories(run_folder);
  // Create a unique name for an abtract unix socket.
  // Using the "@" allows us to create the socket inside the currently running
  // directory We do not need to provide the full path to the "unix:@<SOCKET>"
  // URL to GRPC, but this URL still has a 107 character length limit
  auto socket_name = fmt::format("sysemu_{}_{}.sock", pid, rand);

  RTINFO << "Abstract unit socket name: " + socket_name << "\n";

  // Use an abstract socket and not an actual file
  auto socket_dir = absl::GetFlag(FLAGS_sysemu_socket_dir);
  if (!socket_dir.empty()) {
    auto sock_path = fs::path(socket_dir) / socket_name;
    socket_path_ = std::string("unix://") + sock_path.string();
  } else {
    socket_path_ = std::string("unix:@") + socket_name;
  }

  // Check that the file path we provide can fit in the max length of a socket to be passed to GRPC
  sockaddr_un fsocket;
  assert(socket_path_.size() < sizeof(fsocket.sun_path));

  // Extract hyperparameters from single string
  std::string additional_params_str = absl::GetFlag(FLAGS_sysemu_params);
  std::istringstream iss(additional_params_str);
  std::vector<std::string> additional_params(std::istream_iterator<std::string>{iss},
                                             std::istream_iterator<std::string>());

  sys_emu_ = make_unique<SysEmuLauncher>(absl::GetFlag(FLAGS_sysemu_path),
                                         run_folder,
                                         socket_path_,
                                         absl::GetFlag(FLAGS_sysemu_max_cycles),
                                         absl::GetFlag(FLAGS_sysemu_shires_mask),
                                         absl::GetFlag(FLAGS_sysemu_pu_uart0_tx_file),
                                         absl::GetFlag(FLAGS_sysemu_pu_uart1_tx_file),
                                         absl::GetFlag(FLAGS_sysemu_spio_uart0_tx_file),
                                         absl::GetFlag(FLAGS_sysemu_spio_uart1_tx_file),
                                         additional_params);
};

SysEmuTargetMM::~SysEmuTargetMM() {}

bool SysEmuTargetMM::init() {

  if (device_alive_) {
    return false;
  }

  auto init_grpc = RPCTargetMM::init();
  assert(init_grpc);

  RTINFO << "Starting SysEmu Process";
  RTINFO << "SysEmu socket: " << socket_path_;

  // Set the path that we will use for the socket.
  DLOG(INFO) << "Initializing " << name_ << " to socket path: " << socket_path_;

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

bool SysEmuTargetMM::postFWLoadInit() {
  auto success = RPCTargetMM::postFWLoadInit();
  assert(success);
  return true;
}

bool SysEmuTargetMM::deinit() {
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

bool SysEmuTargetMM::alive() {
  auto status = simulator_status_.wait_for(0ms);

  // check if the simulator has startedy
  if (status == std::future_status::ready) {
    simulator_status_.wait();
    return false;
  }
  return device_alive_;
}

bool SysEmuTargetMM::getStatus() {
  assert(true);
  return false;
}

DeviceInformation SysEmuTargetMM::getStaticConfiguration() {
  assert(true);
  return {};
}

bool SysEmuTargetMM::submitCommand() {
  assert(true);
  return false;
}

bool SysEmuTargetMM::registerResponseCallback() {
  assert(true);
  return false;
}

bool SysEmuTargetMM::registerDeviceEventCallback() {
  assert(true);
  return false;
}
} // namespace device
} // namespace et_runtime
