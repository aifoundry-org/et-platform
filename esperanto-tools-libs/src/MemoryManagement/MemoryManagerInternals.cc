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

MemoryManagerInternals::MemoryManagerInternals(uint64_t dram_base_addr,
                                               uint64_t code_size,
                                               uint64_t data_size)
    : dram_base_addr_(dram_base_addr),
      code_region_(new LinearAllocator(dram_base_addr, code_size)),
      data_region_(
          new BidirectionalAllocator(dram_base_addr + code_size, data_size)) {}

ErrorOr<std::tuple<BufferID, BufferOffsetTy>>
MemoryManagerInternals::mallocCode(BufferSizeTy size, BufferSizeTy alignment) {
  TRACE_MemoryManager_MemoryManagerInternals_mallocCode(size);
  return code_region_->malloc(BufferType::Code, size, alignment);
}

ErrorOr<std::tuple<BufferID, BufferOffsetTy>>
MemoryManagerInternals::emplaceCode(BufferOffsetTy offset, BufferSizeTy size) {
  TRACE_MemoryManager_MemoryManagerInternals_emplaceCode(offset, size);
  return code_region_->emplace(BufferType::Code, offset, size);
}
ErrorOr<std::tuple<BufferID, BufferOffsetTy>>
MemoryManagerInternals::mallocConstant(BufferSizeTy size,
                                       BufferSizeTy alignment) {
  TRACE_MemoryManager_MemoryManagerInternals_mallocConstant(size);
  return data_region_->mallocFront(BufferType::Constant, size, alignment);
}

ErrorOr<std::tuple<BufferID, BufferOffsetTy>>
MemoryManagerInternals::mallocPlaceholder(BufferSizeTy size,
                                          BufferSizeTy alignment) {
  TRACE_MemoryManager_MemoryManagerInternals_mallocPlaceholder(size);
  return data_region_->mallocBack(BufferType::Placeholder, size, alignment);
}

bool MemoryManagerInternals::dataBufferExists(BufferID id) const {
  return data_region_->bufferExists(id);
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

bool MemoryManagerInternals::runSanityCheck() const {
  bool res = true;
  res = res && code_region_->sanityCheck();
  res = res && data_region_->sanityCheck();
  return res;
}

void MemoryManagerInternals::printState() const {
  code_region_->printState();
  data_region_->printState();
}

void MemoryManagerInternals::recordState() const {
  code_region_->stateJSON();
  data_region_->stateJSON();
}
