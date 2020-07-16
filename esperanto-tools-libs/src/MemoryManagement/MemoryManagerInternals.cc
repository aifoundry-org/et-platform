//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "MemoryManagerInternals.h"

#include "BidirectionalAllocator.h"
#include "BufferInfo.h"
#include "LinearAllocator.h"
#include "Tracing/Tracing.h"

#include <algorithm>
#include <iostream>

using namespace et_runtime;
using namespace et_runtime::device::memory_management;

MemoryManagerInternals::MemoryManagerInternals(uint64_t code_size,
                                               uint64_t data_size)
    : code_region_(new LinearAllocator(0, code_size)),
      data_region_(new BidirectionalAllocator(code_size, data_size)) {}

ErrorOr<BufferID> MemoryManagerInternals::mallocCode(BufferSizeTy size) {
  TRACE_MemoryManager_MemoryManagerInternals_mallocCode(size);
  return code_region_->malloc(BufferType::Code, size);
}

ErrorOr<BufferID> MemoryManagerInternals::mallocConstant(BufferSizeTy size) {
  TRACE_MemoryManager_MemoryManagerInternals_mallocConstant(
      size) return data_region_->mallocFront(BufferType::Constant, size);
}

ErrorOr<BufferID> MemoryManagerInternals::mallocPlaceholder(BufferSizeTy size) {
  TRACE_MemoryManager_MemoryManagerInternals_mallocPlaceholder(size);
  return data_region_->mallocBack(BufferType::Placeholder, size);
}

etrtError MemoryManagerInternals::freeCode(BufferID tid) {
  TRACE_MemoryManager_MemoryManagerInternals_freeCode(tid);
  return code_region_->free(tid);
}

etrtError MemoryManagerInternals::freeData(BufferID tid) {
  TRACE_MemoryManager_MemoryManagerInternals_freeData(tid);
  return data_region_->free(tid);
}

uint64_t MemoryManagerInternals::freeMemory() {
  return code_region_->freeMemory() + data_region_->freeMemory();
}

void MemoryManagerInternals::printState() {
  code_region_->printState();
  data_region_->printState();
}
