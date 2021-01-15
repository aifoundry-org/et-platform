/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once
#include "ITarget.h"
#include "PCIEDevice/PCIeDevice.h"

namespace rt {
class TargetSilicon : public ITarget {
public:
  explicit TargetSilicon();
  ~TargetSilicon();

  // ITarget
  std::vector<DeviceId> getDevices() const override;
  size_t getDramSize() const override;
  uint64_t getDramBaseAddr() const override;
  bool writeDevMemDMA(uintptr_t dev_addr, size_t size, const void* buf) override;
  bool readDevMemDMA(uintptr_t dev_addr, size_t size, void* buf) override;
  bool writeMailbox(const void* src, size_t size) override;
  bool readMailbox(std::byte* dst, size_t size) override;
  bool writeDevMemMMIO(uintptr_t dev_addr, size_t size, const void* buf) override;

private:
  std::unique_ptr<et_runtime::device::PCIeDevice> device_;
};
} // namespace rt
