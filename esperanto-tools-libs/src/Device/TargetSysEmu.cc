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

namespace et_runtime {
namespace device {
TargetSysEmu::TargetSysEmu() : DeviceTarget(){};

bool TargetSysEmu::init() {
  assert(true);
  return false;
}

bool TargetSysEmu::deinit() {
  assert(true);
  return false;
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

bool TargetSysEmu::defineDevMem(uintptr_t dev_addr, size_t size, bool is_exec) {
  assert(true);
  return true;
}

bool TargetSysEmu::readDevMem(uintptr_t dev_addr, size_t size, void *buf) {
  assert(true);
  return true;
}

bool TargetSysEmu::writeDevMem(uintptr_t dev_addr, size_t size,
                               const void *buf) {
  assert(true);
  return true;
}
bool TargetSysEmu::launch(uintptr_t launch_pc) {
  assert(true);
  return true;
}

bool TargetSysEmu::boot(uintptr_t init_pc, uintptr_t trap_pc) {
  assert(true);
  return true;
}
}
} // namespace et_runtime
