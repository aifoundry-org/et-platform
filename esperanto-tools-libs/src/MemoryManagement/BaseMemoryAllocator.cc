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

BufferSizeTy BaseMemoryAllocator::mdSize(BufferType type) {
  switch (type) {
  case BufferType::Free:
    return BufferInfo<FreeRegion>::mdSize();
    break;
  case BufferType::Code:
    return BufferInfo<CodeBuffer>::mdSize();
    break;
  case BufferType::Constant:
    return BufferInfo<ConstantBuffer>::mdSize();
    break;
  case BufferType::Placeholder:
    return BufferInfo<PlaceholderBuffer>::mdSize();
    break;
  case BufferType::Logging:
    return BufferInfo<LoggingBuffer>::mdSize();
    break;
  default:
    RTERROR << "Unknown buffer Info \n";
    std::terminate();
    break;
  }
  return -1;
}

std::shared_ptr<AbstractBufferInfo>
BaseMemoryAllocator::createBufferInfo(BufferType type, BufferOffsetTy base,
                                      BufferSizeTy size) {
  switch (type) {
  case BufferType::Free:
    return std::make_shared<BufferInfo<FreeRegion>>(base, size);
    break;
  case BufferType::Code:
    return std::make_shared<BufferInfo<CodeBuffer>>(base, size);
    break;
  case BufferType::Constant:
    return std::make_shared<BufferInfo<ConstantBuffer>>(base, size);
    break;
  case BufferType::Placeholder:
    return std::make_shared<BufferInfo<PlaceholderBuffer>>(base, size);
    break;
  case BufferType::Logging:
    return std::make_shared<BufferInfo<LoggingBuffer>>(base, size);
    break;
  default:
    RTERROR << "Unknown buffer Info \n";
    std::terminate();
    break;
  }
  return std::make_shared<BufferInfo<LoggingBuffer>>(base, size);
}

} // namespace memory_management
} // namespace device
} // namespace et_runtime
