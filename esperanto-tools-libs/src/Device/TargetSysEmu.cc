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

#include <Support/Logging.h>
#include <cstdio>
#include <experimental/filesystem>
#include <string>
#include <string_view>
#include <thread>
namespace fs = std::experimental::filesystem;

using namespace std;
using namespace simulator_api;

namespace et_runtime {
namespace device {
TargetSysEmu::TargetSysEmu(const std::string &path) : RPCTarget(path) {
  // Create a temporary file for the socket and overwrite the
  // one passed by the user. The following function opens the file and
  // immediately deletes as log as it is open by the process it is still
  // accessible.
  socket_file_ = std::tmpfile();
  // Get filename from the socket number
  auto file_path =
      fs::path("/proc/self/fd") / std::to_string(fileno(socket_file_));
  cout << "FilePath : " + file_path.string() << "\n";
  auto socket_name = fs::read_symlink(file_path).string();
  // Remove the following suffix  and replace it with -sysemu.sock
  auto pos = socket_name.find_last_not_of(" (deleted)");
  socket_name = socket_name.substr(0, pos);
  socket_name += "-sysemu.sock";
  path_ = string("unix://") + socket_name;
  sys_emu_ = make_unique<SysEmuLauncher>(path_, vector<string>({"-sim_api"}));
};

TargetSysEmu::~TargetSysEmu() { fclose(socket_file_); }

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

bool TargetSysEmu::deinit() {
  /// If the simulator is not alive any more return
  if (!this->alive()) {
    return false;
  }
  // If ti is alive then send a shut down command
  auto res = this->shutdown();
  assert(res);
  // Wait until we have killed the simulator
  simulator_status_.wait();
  return true;
}

bool TargetSysEmu::alive() {
  auto status = simulator_status_.wait_for(0ms);

  if (status == std::future_status::ready) {
    simulator_status_.wait();
    return false;
  }
  return true;
}

bool TargetSysEmu::getStatus() {
  assert(true);
  return false;
}

bool TargetSysEmu::getStaticConfiguration() {
  assert(true);
  return false;
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

bool TargetSysEmu::boot(uintptr_t init_pc, uintptr_t trap_pc) {
  simulator_api::Request request;
  auto card_emu = new CardEmuReq();
  // Send continue request
  auto cont = new CardEmuContinueReq();
  cont->set_cont(true);
  card_emu->set_allocated_cont(cont);
  request.set_allocated_card_emu(card_emu);
  // Do RPC
  auto reply = doRPC(request);
  assert(reply.has_card_emu());
  auto &card_emu_resp = reply.card_emu();
  assert(card_emu_resp.has_cont());
  auto &cont_resp = card_emu_resp.cont();
  assert(cont_resp.success());
  return true;
}
}
} // namespace et_runtime
