/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <cstdint>

#include <etsoc/common/utils.h>
#include <etsoc/isa/tensors.h>

#include "CommonCode.h"
#include "entryPoint.h"
#include "gpsdk_erbium_sim_access_violation.h"
#include "gpsdk_star_scratchpad.h"

using gpsdk::examples::erbium_sim_access_violation::AccessMode;
using gpsdk::examples::erbium_sim_access_violation::KernelArguments;
using gpsdk::examples::erbium_sim_access_violation::Result;

int entryPoint(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint, nullptr);

namespace {

constexpr uint64_t kIllegalOffset = gpsdk::star_scratchpad::kPoolBaseOffset;
constexpr uint64_t kTensorStrideBytes = 64ULL;

} // namespace

int entryPoint(KernelArguments* args) {
  et_assert(args != nullptr);
  et_assert(args->resultAddress != 0ULL);

  if (get_relative_thread_id() != 0) {
    return 0;
  }

  auto* result = reinterpret_cast<volatile Result*>(args->resultAddress);
  result->magic = gpsdk::examples::erbium_sim_access_violation::kResultMagic;
  result->started = 1U;
  result->completed = 0U;
  result->targetShire = args->targetShire;
  result->accessMode = args->accessMode;
  result->centerShire = getEffectiveCenterShire();
  result->observedWord = 0U;

  const auto illegalAddress = gpsdk::star_scratchpad::format0Address(args->targetShire, kIllegalOffset);

  switch (static_cast<AccessMode>(args->accessMode)) {
  case AccessMode::AtomicRead: {
    result->observedWord =
      atomic_load_global_32(reinterpret_cast<volatile const uint32_t*>(illegalAddress));
    break;
  }
  case AccessMode::GlobalMemcpy: {
    alignas(32) uint32_t buffer[8] = {};
    global_memcpy(buffer, reinterpret_cast<const void*>(illegalAddress), sizeof(buffer));
    result->observedWord = buffer[0];
    break;
  }
  case AccessMode::TensorLoad: {
    alignas(64) float tile[16 * 16] = {};
    tensor_load(false, false, 0U, 0U, 0U, illegalAddress, 0U, 15U, kTensorStrideBytes, 0U);
    tensor_wait(TENSOR_LOAD_WAIT_0);
    const auto* words = reinterpret_cast<const uint32_t*>(tile);
    result->observedWord = words[0];
    break;
  }
  }

  result->completed = 1U;
  return 0;
}
