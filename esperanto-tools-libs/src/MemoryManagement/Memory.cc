//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/Core/Memory.h"
#include "esperanto/runtime/Core/MemoryManager.h"
#include "esperanto/runtime/Support/Logging.h"

namespace et_runtime {

DeviceBuffer::DeviceBuffer()
    : buffer_id_(0), offset_(0), size_(0), ref_cntr_(nullptr),
      deallocator_(nullptr) {}

DeviceBuffer::DeviceBuffer(BufferID id, BufferOffsetTy offset,
                           BufferSizeTy size, Deallocator *deallocator)
    : buffer_id_(id), offset_(offset), size_(size), ref_cntr_(new RefCounter()),
      deallocator_(deallocator) {
  ref_cntr_->inc();
}

DeviceBuffer::~DeviceBuffer() {
  auto res = ReleaseBuffer();
  if (!res) {
    RTERROR << "Failed to de-allocate buffer: " << buffer_id_ << "\n";
    std::terminate();
  }
}

DeviceBuffer::DeviceBuffer(const DeviceBuffer &other) {
  copyBuffer(other);
  ref_cntr_->inc();
}

DeviceBuffer::DeviceBuffer(DeviceBuffer &&other) {
  copyBuffer(other);
  other.zeroFields();
}

DeviceBuffer &DeviceBuffer::operator=(DeviceBuffer &other) {
  ReleaseBuffer();
  copyBuffer(other);
  ref_cntr_->inc();
  return *this;
}

DeviceBuffer &DeviceBuffer::operator=(DeviceBuffer &&other) {
  ReleaseBuffer();
  copyBuffer(other);
  other.zeroFields();
  return *this;
}

bool DeviceBuffer::unique() const { return ref_cntr_->val() == 0; }

DeviceBuffer::operator bool() const { return buffer_id_ == 0; }

void DeviceBuffer::copyBuffer(const DeviceBuffer &other) {
  buffer_id_ = other.buffer_id_;
  offset_ = other.offset_;
  size_ = other.size_;
  ref_cntr_ = other.ref_cntr_;
  deallocator_ = other.deallocator_;
}

void DeviceBuffer::zeroFields() {
  buffer_id_ = 0;
  offset_ = 0;
  size_ = 0;
  ref_cntr_ = nullptr;
  deallocator_ = nullptr;
}

bool DeviceBuffer::ReleaseBuffer() {
  if (ref_cntr_) {
    ref_cntr_->dec();
    if (ref_cntr_->val() == 0) {
      delete ref_cntr_;
      ref_cntr_ = nullptr;

      auto res = (*deallocator_)(buffer_id_);
      deallocator_ = nullptr;
      return (res == etrtSuccess);
    }
  }
  return true;
}

bool operator<(const et_runtime::DeviceBuffer &a,
               const et_runtime::DeviceBuffer &b) {
  return a.id() < b.id();
}

bool operator==(const et_runtime::DeviceBuffer &a,
                const et_runtime::DeviceBuffer &b) {
  return a.id() == b.id();
}

} // namespace et_runtime
