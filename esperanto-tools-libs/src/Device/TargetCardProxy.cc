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
#include "SysEmuLauncher.h"

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
  sys_emu_ = make_unique<SysEmuLauncher>(connection_);
  card_proxy_ = std::make_unique<CardProxy>(connection_);
}

CardProxyTarget::~CardProxyTarget() {}

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
  simulator_status_ = async(launch::async, [this]() -> auto {
    return this->sys_emu_->launchSimulator();
  });

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

bool CardProxyTarget::defineDevMem(uintptr_t dev_addr, size_t size,
                                   bool is_exec) {
  cpDefineDevMem(card_proxy_.get(), dev_addr, size, is_exec);
  return true;
}

bool CardProxyTarget::readDevMem(uintptr_t dev_addr, size_t size, void *buf) {
  cpReadDevMem(card_proxy_.get(), dev_addr, size, buf);
  return true;
}

bool CardProxyTarget::writeDevMem(uintptr_t dev_addr, size_t size,
                                  const void *buf) {
  cpWriteDevMem(card_proxy_.get(), dev_addr, size, buf);
  return true;
}
bool CardProxyTarget::launch(uintptr_t launch_pc, const layer_dynamic_info *params) {
      fprintf(stderr,
            "CardProxyTarget::Going to execute kernel {0x%lx}\n"
            "  tensor_a = 0x%" PRIx64 "\n"
            "  tensor_b = 0x%" PRIx64 "\n"
            "  tensor_c = 0x%" PRIx64 "\n"
            "  tensor_d = 0x%" PRIx64 "\n"
            "  tensor_e = 0x%" PRIx64 "\n"
            "  tensor_f = 0x%" PRIx64 "\n"
            "  tensor_g = 0x%" PRIx64 "\n"
            "  tensor_h = 0x%" PRIx64 "\n"
            "  pc/id    = 0x%" PRIx64 "\n",
            launch_pc,
            params->tensor_a, params->tensor_b, params->tensor_c,
            params->tensor_d, params->tensor_e, params->tensor_f,
            params->tensor_g, params->tensor_h, params->kernel_id);

  RTINFO << "SysEmu launch CardProxyTarget.";
  cpLaunch(card_proxy_.get(), launch_pc, params);
  return true;
}

bool CardProxyTarget::boot(uintptr_t init_pc, uintptr_t trap_pc) {
  cpBoot(card_proxy_.get(), init_pc, trap_pc);
  return true;
}

} // namespace device
} // namespace et_runtime
