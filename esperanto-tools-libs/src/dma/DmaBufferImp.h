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
#include "IDmaBuffer.h"
#include "Utils.h"

#include <device-layer/IDeviceLayer.h>
#include <runtime/IRuntime.h>

#include <memory>

namespace rt {
class DmaBufferImp : public IDmaBuffer {
public:
  DmaBufferImp(int device, size_t size, bool writeable, std::shared_ptr<dev::IDeviceLayer> const& deviceLayer)
    : deviceLayer_(deviceLayer)
    , size_(size)
    , address_(reinterpret_cast<std::byte*>(deviceLayer_->allocDmaBuffer(device, size, writeable))) {
    RT_VLOG(MID) << "Allocated dma buffer: " << std::hex << address_ << " size: " << size_ << " writeble? "
                 << (writeable ? "True" : "False");
  }
  ~DmaBufferImp() {
    if (address_) {
      RT_VLOG(MID) << "Deallocating dma buffer: " << std::hex << address_ << " size: " << size_;
      deviceLayer_->freeDmaBuffer(address_);
      address_ = nullptr;
    }
  }

  std::byte* getPtr() const override {
    return address_;
  }
  size_t getSize() const override {
    return size_;
  }

private:
  std::shared_ptr<dev::IDeviceLayer> deviceLayer_;
  size_t size_;
  std::byte* address_;
};
} // namespace rt
