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

std::shared_ptr<AbstractBufferInfo> BaseMemoryAllocator::createBufferInfo(
    BufferType type, BufferOffsetTy base, BufferOffsetTy aligned_start,
    BufferSizeTy req_size, BufferSizeTy size) {
  switch (type) {
  case BufferType::Free:
    return std::make_shared<BufferInfo<FreeRegion>>(base, aligned_start,
                                                    req_size, size);
    break;
  case BufferType::Code:
    return std::make_shared<BufferInfo<CodeBuffer>>(base, aligned_start,
                                                    req_size, size);
    break;
  case BufferType::Constant:
    return std::make_shared<BufferInfo<ConstantBuffer>>(base, aligned_start,
                                                        req_size, size);
    break;
  case BufferType::Placeholder:
    return std::make_shared<BufferInfo<PlaceholderBuffer>>(base, aligned_start,
                                                           req_size, size);
    break;
  case BufferType::Logging:
    return std::make_shared<BufferInfo<LoggingBuffer>>(base, aligned_start,
                                                       req_size, size);
    break;
  default:
    RTERROR << "Unknown buffer Info \n";
    std::terminate();
    break;
  }
  return std::make_shared<BufferInfo<LoggingBuffer>>(base, aligned_start,
                                                     req_size, size);
}

BufferSizeTy BaseMemoryAllocator::alignmentFixSize(BufferSizeTy size,
                                                   BufferSizeTy md_size,
                                                   BufferSizeTy alignment) {
  auto total_size = md_size + size;
  if (alignment > 0) {
    total_size = (((total_size + (alignment - 1)) / alignment) + 1) * alignment;
  }
  return total_size;
}

BufferSizeTy BaseMemoryAllocator::alignedStart(BufferOffsetTy base,
                                               BufferSizeTy mdSize,
                                               BufferSizeTy alignment) {
  auto aligned_buffer = base + mdSize;
  if (alignment > 0) {
    aligned_buffer =
        (((aligned_buffer + (alignment - 1)) / alignment)) * alignment;
  }
  assert(aligned_buffer - mdSize >= base);
  return aligned_buffer;
}

} // namespace memory_management
} // namespace device
} // namespace et_runtime
