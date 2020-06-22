//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TensorInfo.h"

namespace et_runtime {
namespace device {
namespace memory_management {

TensorID nextTensorID() {
  static TensorID lastid = 0;
  return lastid++;
}

template <> const TensorType TensorInfo<FreeRegion>::type() {
  return TensorType::Free;
}

template <>
const TensorPermissionsTy TensorInfo<FreeRegion>::permissions() const {
  return static_cast<TensorPermissionsTy>(TensorPermissions::None);
}

template <> const TensorType TensorInfo<CodeBuffer>::type() {
  return TensorType::Code;
}

template <>
const TensorPermissionsTy TensorInfo<CodeBuffer>::permissions() const {
  return TensorPermissions::Read | TensorPermissions::Execute;
}

template <> const TensorType TensorInfo<ConstantTensor>::type() {
  return TensorType::Constant;
}

template <>
const TensorPermissionsTy TensorInfo<ConstantTensor>::permissions() const {
  return static_cast<TensorPermissionsTy>(TensorPermissions::Read);
}

template <> const TensorType TensorInfo<PlaceholderTensor>::type() {
  return TensorType::Placeholder;
}

template <>
const TensorPermissionsTy TensorInfo<PlaceholderTensor>::permissions() const {
  return TensorPermissions::Read | TensorPermissions::Write;
}

template <> const TensorType TensorInfo<LoggingBuffer>::type() {
  return TensorType::Logging;
}

template <>
const TensorPermissionsTy TensorInfo<LoggingBuffer>::permissions() const {
  return TensorPermissions::Read | TensorPermissions::Write;
}

std::ostream &operator<<(std::ostream &os, const AbstractTensorInfo &t) {
  os << "Tensor: " << std::dec << t.id() //.
     << " MDBase: " << t.mdBase()        //
     << " Base: " << t.base()            //
     << " Size: " << t.size();
  return os;
}

} // namespace memory_management
} // namespace device
} // namespace et_runtime
