//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_PCIE_H
#define ET_RUNTIME_DEVICE_PCIE_H

#include "Core/DeviceTarget.h"

#include "CharDevice.h"
#include "MailBoxDev.h"

#include <cassert>
#include <string>

namespace et_runtime {
namespace device {

class PCIeDevice final : public DeviceTarget {
public:
  PCIeDevice(int index);
  ~PCIeDevice() = default;

  static std::vector<DeviceInformation> enumerateDevices();

  bool init() override;
  bool deinit() override;
  bool getStatus() override;
  DeviceInformation getStaticConfiguration() override;
  bool submitCommand() override;
  bool registerResponseCallback() override;
  bool registerDeviceEventCallback() override;
  bool readDevMem(uintptr_t dev_addr, size_t size, void *buf) override;
  bool writeDevMem(uintptr_t dev_addr, size_t size, const void *buf) override;
  bool launch(uintptr_t launch_pc, const layer_dynamic_info *params) override;
  bool boot(uintptr_t init_pc, uintptr_t trap_pc) override;

private:
  int index_;
  std::string prefix_;
  CharacterDevice bulk_;
  MailBoxDev from_mm_;
  MailBoxDev to_mm_;
  MailBoxDev from_sp_;
  MailBoxDev to_sp_;
  CharacterDevice pcie_userersr_;
  CharacterDevice trg_pcie_;
  CharacterDevice mbox_sp_;
  CharacterDevice mbox_mm_;
  CharacterDevice drct_dram_;
};

} // namespace device
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_PCIE_H
