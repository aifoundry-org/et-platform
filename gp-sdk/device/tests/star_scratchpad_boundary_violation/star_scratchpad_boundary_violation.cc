/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <cstddef>
#include <cstdint>

#include "CommonCode.h"
#include "StarScratchpadPool.h"
#include "entryPoint.h"
#include "gpsdk_star_scratchpad.h"
#include "gpsdk_star_scratchpad_boundary_violation.h"

using gpsdk::examples::star_scratchpad_boundary_violation::KernelArguments;
using gpsdk::examples::star_scratchpad_boundary_violation::Result;
using gpsdk::examples::star_scratchpad_boundary_violation::ViolationMode;

int entryPoint(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint, nullptr);

namespace {

constexpr uint64_t kOversizeBytes = gpsdk::star_scratchpad::kPoolBytesPerAuxShire + 64ULL;
constexpr uint64_t kCrossShardOffset = gpsdk::star_scratchpad::kPoolBytesPerAuxShire - 32ULL;
constexpr uint64_t kCrossShardBytes = 64ULL;

} // namespace

int entryPoint(KernelArguments* args) {
  et_assert(args != nullptr);
  et_assert(args->resultAddress != 0ULL);

  if (get_relative_thread_id() != 0) {
    return 0;
  }

  auto* result = reinterpret_cast<volatile Result*>(args->resultAddress);
  result->magic = gpsdk::examples::star_scratchpad_boundary_violation::kResultMagic;
  result->started = 1U;
  result->completed = 0U;
  result->violationMode = args->violationMode;
  result->logicalOffset = 0ULL;
  result->requestedBytes = 0ULL;
  result->observedAddress = 0ULL;

  switch (static_cast<ViolationMode>(args->violationMode)) {
  case ViolationMode::OversizeContiguousRequest: {
    result->logicalOffset = 0ULL;
    result->requestedBytes = kOversizeBytes;
    auto* ptr = gpsdk::device::star_scratchpad::ptr<std::byte>(result->logicalOffset, result->requestedBytes);
    result->observedAddress = reinterpret_cast<uint64_t>(ptr);
    break;
  }
  case ViolationMode::CrossShardContiguousRequest: {
    result->logicalOffset = kCrossShardOffset;
    result->requestedBytes = kCrossShardBytes;
    auto* ptr = gpsdk::device::star_scratchpad::ptr<std::byte>(result->logicalOffset, result->requestedBytes);
    result->observedAddress = reinterpret_cast<uint64_t>(ptr);
    break;
  }
  }

  result->completed = 1U;
  return 0;
}
