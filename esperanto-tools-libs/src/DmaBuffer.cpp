/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "runtime/DmaBuffer.h"
#include "DmaBufferImp.h"
#include "DmaBufferManager.h"
#include "RuntimeImp.h"
#include "utils.h"

namespace rt {

const std::byte* DmaBuffer::getPtr() const {
  return impl_->address_;
}
std::byte* DmaBuffer::getPtr() {
  return impl_->address_;
}
size_t DmaBuffer::getSize() const {
  return impl_->size_;
}
bool DmaBuffer::containsAddr(const std::byte* address) const {
  return impl_->containsAddr(address);
}

DmaBuffer::DmaBuffer(DmaBuffer&&) = default;
DmaBuffer& DmaBuffer::operator=(DmaBuffer&&) = default;

DmaBuffer::DmaBuffer(std::unique_ptr<DmaBufferImp> impl, DmaBufferManager* dmaBufferManager)
  : impl_(std::move(impl))
  , dmaBufferManager_(dmaBufferManager) {
  LOG(INFO) << "Dma buffer manager at ctor: " << dmaBufferManager_;
}

DmaBuffer::~DmaBuffer() {
  // call to deallocate DmaBufferImp
  if (impl_) {
    auto dmaBuffer = impl_.release();
    LOG(INFO) << "Dma buffer manager at dtor: " << dmaBufferManager_;
    dmaBufferManager_->release(dmaBuffer);
  }
}
} // namespace rt