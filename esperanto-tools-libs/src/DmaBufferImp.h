/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#pragma once
#include <device-layer/IDeviceLayer.h>
#include <runtime/DmaBuffer.h>
#include <runtime/IRuntime.h>

namespace rt {
struct DmaBufferImp {
  explicit DmaBufferImp(int device, size_t size, bool writeable, dev::IDeviceLayer* deviceLayer)
    : address_(reinterpret_cast<std::byte*>(deviceLayer->allocDmaBuffer(device, size, writeable)))
    , size_(size) {
  }
  explicit DmaBufferImp() = default;
  bool containsAddr(const std::byte* address) const {
    return address >= address_ && address < address_ + size_;
  }
  bool operator<(const DmaBufferImp& other) const {
    return address_ < other.address_;
  }
  std::byte* address_;
  size_t size_;
};
} // namespace rt
