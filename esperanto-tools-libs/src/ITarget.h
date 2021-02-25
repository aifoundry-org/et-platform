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

#include "runtime/IRuntime.h"
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace rt {
class ITarget {
public:
  virtual std::vector<DeviceId> getDevices() const = 0;

  virtual uint64_t getDramBaseAddr() const = 0;
  virtual size_t getDramSize() const = 0;
  virtual bool writeDevMemDMA(uintptr_t dev_addr, size_t size, const void* buf) = 0;
  virtual bool readDevMemDMA(uintptr_t dev_addr, size_t size, void* buf) = 0;
  virtual bool writeMailbox(const void* src, size_t size) = 0;
  virtual bool readMailbox(std::byte* dst, size_t size) = 0;
  virtual bool writeDevMemMMIO(uintptr_t dev_addr, size_t size, const void* buf) = 0;

  virtual ~ITarget() = default;
  // TODO. there are a lot of missing stuff I will be adding when I need it
};
} // namespace rt
