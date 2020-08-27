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

#include "esperanto/runtime/Core/DeviceTarget.h"

/// @file

#include "BulkDev.h"
#include "MailBoxDev.h"

#include <cassert>
#include <string>

namespace et_runtime {
namespace device {

/// @class PCIeDevice
class PCIeDevice final : public DeviceTarget {
public:
  PCIeDevice(int index);
  ~PCIeDevice() = default;

  static std::vector<DeviceInformation> enumerateDevices();

  TargetType type() override { return DeviceTarget::TargetType::PCIe; }
  bool init() override;
  bool postFWLoadInit() override {
    // Do nothing
    return true;
  }
  bool deinit() override;
  bool getStatus() override;
  DeviceInformation getStaticConfiguration() override;
  bool submitCommand() override;
  bool registerResponseCallback() override;
  bool registerDeviceEventCallback() override;
  bool readDevMemMMIO(uintptr_t dev_addr, size_t size, void *buf) final;
  bool writeDevMemMMIO(uintptr_t dev_addr, size_t size, const void *buf) final;
  bool readDevMemDMA(uintptr_t dev_addr, size_t size, void *buf) final;
  bool writeDevMemDMA(uintptr_t dev_addr, size_t size, const void *buf) final;
  bool mb_write(const void *data, ssize_t size) final;
  ssize_t mb_read(void *data, ssize_t size,
                  TimeDuration wait_time = TimeDuration::max()) final;
  bool launch() override;
  bool boot(uint64_t pc) override;
  bool shutdown() override;

  /// @brief Return the absolute base DRAM address we can access
  uintptr_t dramBaseAddr() const override;

  /// @brief Return the size of DRAM we can write to in bytes.
  uintptr_t dramSize() const override;

  ssize_t mboxMsgMaxSize() const override;

private:
  int index_;
  std::string prefix_;
  BulkDev bulk_;
  MailBoxDev mm_;
};

} // namespace device
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_PCIE_H
