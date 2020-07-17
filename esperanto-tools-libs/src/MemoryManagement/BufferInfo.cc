//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "BufferInfo.h"

namespace et_runtime {
namespace device {
namespace memory_management {

BufferID nextBufferID() {
  static BufferID lastid = 0;
  return lastid++;
}

template <> const BufferType BufferInfo<FreeRegion>::type() {
  return BufferType::Free;
}

template <>
const BufferPermissionsTy BufferInfo<FreeRegion>::permissions() const {
  return static_cast<BufferPermissionsTy>(BufferPermissions::None);
}

template <> const BufferType BufferInfo<CodeBuffer>::type() {
  return BufferType::Code;
}

template <>
const BufferPermissionsTy BufferInfo<CodeBuffer>::permissions() const {
  return BufferPermissions::Read | BufferPermissions::Execute;
}

template <> const BufferType BufferInfo<ConstantBuffer>::type() {
  return BufferType::Constant;
}

template <>
const BufferPermissionsTy BufferInfo<ConstantBuffer>::permissions() const {
  return static_cast<BufferPermissionsTy>(BufferPermissions::Read);
}

template <> const BufferType BufferInfo<PlaceholderBuffer>::type() {
  return BufferType::Placeholder;
}

template <>
const BufferPermissionsTy BufferInfo<PlaceholderBuffer>::permissions() const {
  return BufferPermissions::Read | BufferPermissions::Write;
}

template <> const BufferType BufferInfo<LoggingBuffer>::type() {
  return BufferType::Logging;
}

template <>
const BufferPermissionsTy BufferInfo<LoggingBuffer>::permissions() const {
  return BufferPermissions::Read | BufferPermissions::Write;
}

std::ostream &operator<<(std::ostream &os, const AbstractBufferInfo &t) {
  os << "Buffer: " << std::dec << t.id() //.
     << " MDBase: " << t.mdBase()        //
     << " Base: " << t.base()            //
     << " Size: " << t.size();
  return os;
}

} // namespace memory_management
} // namespace device
} // namespace et_runtime
