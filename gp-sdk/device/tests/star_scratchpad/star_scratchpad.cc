/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <etsoc/common/utils.h>
#include <etsoc/isa/atomic.h>
#include <etsoc/isa/hart.h>

#include "CommonCode.h"
#include "entryPoint.h"
#include "sync.h"

class KernelArguments;
int entryPoint(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint, nullptr);

namespace {

constexpr uint32_t kStarCols = 8U;
constexpr uint32_t kStarNeighborCount = 4U;
constexpr uint64_t kScpRegionBaseAddress = 0x80000000ULL;
constexpr uint64_t kScpShireSize = 0x280000ULL;
constexpr uint64_t kProbeBaseOffset = kScpShireSize - 0x4000ULL;
constexpr uint64_t kProbeNeighborStride = 0x400ULL;
constexpr uint64_t kSuccessMarkerOffset = kProbeBaseOffset + 0x2000ULL;
constexpr uint64_t kSuccessMarker = 0x5354415250524F42ULL;

inline uint64_t makeProbeValue(uint32_t centerShire, uint32_t neighborShire, uint32_t relativeThreadId) {
  return (0x5A5A000000000000ULL | (static_cast<uint64_t>(centerShire) << 24) |
          (static_cast<uint64_t>(neighborShire) << 8) | static_cast<uint64_t>(relativeThreadId));
}

inline void computeStarNeighbors(uint32_t centerShire, uint32_t* neighbors) {
  neighbors[0] = centerShire - kStarCols;
  neighbors[1] = centerShire + 1U;
  neighbors[2] = centerShire + kStarCols;
  neighbors[3] = centerShire - 1U;
}

inline volatile uint64_t* getProbeAddress(uint32_t shireId, uint32_t neighborIdx) {
  const auto offset = kProbeBaseOffset + (static_cast<uint64_t>(neighborIdx) * kProbeNeighborStride);
  const auto address = (((static_cast<uint64_t>(shireId) << 23) & 0x3F800000ULL) + kScpRegionBaseAddress + offset);
  return reinterpret_cast<volatile uint64_t*>(address);
}

inline volatile uint64_t* getSuccessMarkerAddress(uint32_t shireId) {
  const auto address = (((static_cast<uint64_t>(shireId) << 23) & 0x3F800000ULL) + kScpRegionBaseAddress +
                        kSuccessMarkerOffset);
  return reinterpret_cast<volatile uint64_t*>(address);
}

} // namespace

int entryPoint([[maybe_unused]] KernelArguments* args) {
  const auto relativeThreadId = get_relative_thread_id();
  if (relativeThreadId != 0) {
    return 0;
  }

  et_assert(isScratchpadStarClusterEnabled());
  et_assert(getActiveMinionsPerShire() == gpsdk::launch::kMinionsPerNeighborhood);
  et_assert(__builtin_popcountll(getComputeShireMask()) == 1);
  et_assert(__builtin_popcountll(getLaunchedShireMask()) == 5);

  const auto centerShire = get_shire_id();
  et_assert(getComputeShireMask() == (1ULL << centerShire));

  uint32_t neighbors[kStarNeighborCount];
  computeStarNeighbors(centerShire, neighbors);

  for (uint32_t idx = 0; idx < kStarNeighborCount; ++idx) {
    atomic_store_global_64(getProbeAddress(neighbors[idx], idx), makeProbeValue(centerShire, neighbors[idx], 0U));
  }

  asm volatile("fence\n" ::: "memory");

  for (uint32_t idx = 0; idx < kStarNeighborCount; ++idx) {
    const auto value = atomic_load_global_64(getProbeAddress(neighbors[idx], idx));
    const auto expected = makeProbeValue(centerShire, neighbors[idx], 0U);
    et_assert(value == expected);
  }

  atomic_store_global_64(getSuccessMarkerAddress(centerShire), kSuccessMarker);

  return 0;
}
