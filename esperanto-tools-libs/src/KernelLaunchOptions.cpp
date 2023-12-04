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

KernelLaunchOptions::KernelLaunchOptions(const rt::KernelLaunchOptionsImp& kOptImp) {
  imp_ = std::make_unique<KernelLaunchOptionsImp>(kOptImp);
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
  UserTrace userTraceConfig{buffer, bufferSize, threshold, shireMask, threadMask, eventMask, filterMask};
  imp_->userTraceConfig_ = userTraceConfig;
}

void KernelLaunchOptions::setStackConfig(std::byte* baseAddress, uint64_t totalSize) {
  if ((reinterpret_cast<uint64_t>(baseAddress) % SIZE_4K) != 0) {
    throw Exception("Stack baseAddress has to be aligned to FOUR_KB");
  }
  if ((totalSize % SIZE_4K) != 0) {
    throw Exception("Stack size has to be aligned to FOUR_KB");
  }

  setIfImpIsNull();
  auto rawAddr = reinterpret_cast<uint64_t>(baseAddress);
  imp_->stackConfig_ = StackConfiguration{rawAddr, totalSize};
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
