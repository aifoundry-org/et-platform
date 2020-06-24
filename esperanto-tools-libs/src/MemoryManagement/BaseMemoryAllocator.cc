//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "BaseMemoryAllocator.h"
#include "esperanto/runtime/Support/HelperMacros.h"

namespace et_runtime {
namespace device {
namespace memory_management {

TensorSizeTy BaseMemoryAllocator::mdSize(TensorType type) {
  switch (type) {
  case TensorType::Free:
    return TensorInfo<FreeRegion>::mdSize();
    break;
  case TensorType::Code:
    return TensorInfo<CodeBuffer>::mdSize();
    break;
  case TensorType::Constant:
    return TensorInfo<ConstantTensor>::mdSize();
    break;
  case TensorType::Placeholder:
    return TensorInfo<PlaceholderTensor>::mdSize();
    break;
  case TensorType::Logging:
    return TensorInfo<LoggingBuffer>::mdSize();
    break;
  default:
    RTERROR << "Unknown tensor Info \n";
    std::terminate();
    break;
  }
  return -1;
}

std::shared_ptr<AbstractTensorInfo>
BaseMemoryAllocator::createTensorInfo(TensorType type, TensorOffsetTy base,
                                      TensorSizeTy size) {
  switch (type) {
  case TensorType::Free:
    return std::make_shared<TensorInfo<FreeRegion>>(base, size);
    break;
  case TensorType::Code:
    return std::make_shared<TensorInfo<CodeBuffer>>(base, size);
    break;
  case TensorType::Constant:
    return std::make_shared<TensorInfo<ConstantTensor>>(base, size);
    break;
  case TensorType::Placeholder:
    return std::make_shared<TensorInfo<PlaceholderTensor>>(base, size);
    break;
  case TensorType::Logging:
    return std::make_shared<TensorInfo<LoggingBuffer>>(base, size);
    break;
  default:
    RTERROR << "Unknown tensor Info \n";
    std::terminate();
    break;
  }
  return std::make_shared<TensorInfo<LoggingBuffer>>(base, size);
}

} // namespace memory_management
} // namespace device
} // namespace et_runtime
