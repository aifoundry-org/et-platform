//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TargetRPC.h"

namespace et_runtime {
namespace device {

RPCTarget::RPCTarget(const std::string &path) : DeviceTarget(path) {}

bool RPCTarget::init() {
  assert(true);
  return false;
}

bool RPCTarget::deinit() {
  assert(true);
  return false;
}

bool RPCTarget::getStatus() {
  assert(true);
  return false;
}

bool RPCTarget::getStaticConfiguration() {
  assert(true);
  return false;
}

bool RPCTarget::submitCommand() {
  assert(true);
  return false;
}

bool RPCTarget::registerResponseCallback() {
  assert(true);
  return false;
}

bool RPCTarget::registerDeviceEventCallback() {
  assert(true);
  return false;
}

bool RPCTarget::defineDevMem(uintptr_t dev_addr, size_t size, bool is_exec) {
  assert(true);
  return true;
}

bool RPCTarget::readDevMem(uintptr_t dev_addr, size_t size, void *buf) {
  assert(true);
  return true;
}

bool RPCTarget::writeDevMem(uintptr_t dev_addr, size_t size, const void *buf) {
  assert(true);
  return true;
}
bool RPCTarget::launch(uintptr_t launch_pc) {
  assert(true);
  return true;
}

bool RPCTarget::boot(uintptr_t init_pc, uintptr_t trap_pc) {
  assert(true);
  return true;
}

} // namespace device
} // namespace et_runtime
