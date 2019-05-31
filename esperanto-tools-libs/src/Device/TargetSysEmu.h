//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_SYSEMU_H
#define ET_RUNTIME_DEVICE_SYSEMU_H

#include "Core/DeviceTarget.h"

#include <cassert>

namespace et_runtime {
namespace device {

class TargetSysEmu final : public DeviceTarget {
public:
  TargetSysEmu();
  virtual ~TargetSysEmu() = default;

  bool init() override;
  bool deinit() override;
  virtual bool getStatus() override;
  virtual bool getStaticConfiguration() override;
  virtual bool submitCommand() override;
  virtual bool registerResponseCallback() override;
  virtual bool registerDeviceEventCallback() override;
  bool defineDevMem(uintptr_t dev_addr, size_t size, bool is_exec) override;
  bool readDevMem(uintptr_t dev_addr, size_t size, void *buf) override;
  bool writeDevMem(uintptr_t dev_addr, size_t size, const void *buf) override;
  bool launch(uintptr_t launch_pc) override;
  bool boot(uintptr_t init_pc, uintptr_t trap_pc) override;

private:
};

} // namespace device
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_SYSEMU_H
