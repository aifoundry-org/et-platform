/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#ifndef GPSDK_STAR_SCRATCHPAD_BOUNDARY_VIOLATION_H
#define GPSDK_STAR_SCRATCHPAD_BOUNDARY_VIOLATION_H

#include <cstdint>

namespace gpsdk::examples::star_scratchpad_boundary_violation {

constexpr uint64_t kResultMagic = 0x535041424E443031ULL; // "SPABND01"

enum class ViolationMode : uint32_t {
  OversizeContiguousRequest = 0U,
  CrossShardContiguousRequest = 1U,
};

struct KernelArguments {
  uint64_t resultAddress = 0ULL;
  uint32_t violationMode = 0U;
};

struct alignas(64) Result {
  uint64_t magic = 0ULL;
  uint32_t started = 0U;
  uint32_t completed = 0U;
  uint32_t violationMode = 0U;
  uint64_t logicalOffset = 0ULL;
  uint64_t requestedBytes = 0ULL;
  uint64_t observedAddress = 0ULL;
};

inline constexpr const char* violationModeName(ViolationMode mode) {
  switch (mode) {
  case ViolationMode::OversizeContiguousRequest:
    return "oversize_contiguous_request";
  case ViolationMode::CrossShardContiguousRequest:
    return "cross_shard_contiguous_request";
  }
  return "unknown";
}

} // namespace gpsdk::examples::star_scratchpad_boundary_violation

#endif // GPSDK_STAR_SCRATCHPAD_BOUNDARY_VIOLATION_H
