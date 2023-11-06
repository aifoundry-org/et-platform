/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include <memory>
#include <optional>

#include "KernelLaunchOptionsImp.h"
#include "runtime/Types.h"

using namespace rt;

KernelLaunchOptions::KernelLaunchOptions() {
}

KernelLaunchOptions::~KernelLaunchOptions() {
}

void KernelLaunchOptions::setShireMask(uint64_t shireMask) {
  setIfImpIsNull();
  imp_->shireMask_ = shireMask;
}

void KernelLaunchOptions::setBarrier(bool barrier) {
  setIfImpIsNull();
  imp_->barrier_ = barrier;
}

void KernelLaunchOptions::setFlushL3(bool flushL3) {
  setIfImpIsNull();
  imp_->flushL3_ = flushL3;
}

void KernelLaunchOptions::setUserTracing(uint64_t buffer, uint32_t bufferSize, uint32_t threshold, uint64_t shireMask,
                                         uint64_t threadMask, uint32_t eventMask, uint32_t filterMask) {
  setIfImpIsNull();
  imp_->userTraceConfig_->buffer_ = buffer;
  imp_->userTraceConfig_->buffer_size_ = bufferSize;
  imp_->userTraceConfig_->threshold_ = threshold;
  imp_->userTraceConfig_->shireMask_ = shireMask;
  imp_->userTraceConfig_->threadMask_ = threadMask;
  imp_->userTraceConfig_->eventMask_ = eventMask;
  imp_->userTraceConfig_->filterMask_ = filterMask;
}

void KernelLaunchOptions::setStackConfig(std::byte* baseAddress, uint64_t size) {
  setIfImpIsNull();
  imp_->stackConfig_->baseAddress_ = baseAddress;
  imp_->stackConfig_->size_ = size;
}

void KernelLaunchOptions::setCoreDumpFilePath(const std::string& coreDumpFilePath) {
  setIfImpIsNull();
  imp_->coreDumpFilePath_ = coreDumpFilePath;
}

void KernelLaunchOptions::setIfImpIsNull(void) {
  if (imp_ == nullptr) {
    imp_ = std::make_unique<KernelLaunchOptionsImp>(DefaultKernelOptions::defaultKernelOptions);
  }
}