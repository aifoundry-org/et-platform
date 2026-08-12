/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <etsoc/common/utils.h>
#include <etsoc/isa/atomic.h>
#include <etsoc/isa/hart.h>

#include "CommonCode.h"
#include "StarScratchpadPool.h"
#include "entryPoint.h"
#include "gpsdk_star_scratchpad.h"
#include "sync.h"

class KernelArguments;
int entryPoint(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint, nullptr);

namespace {

constexpr uint32_t kStarNeighborCount = 4U;

inline uint64_t makeProbeValue(uint32_t centerShire, uint32_t neighborShire, uint32_t relativeThreadId) {
  return (0x5A5A000000000000ULL | (static_cast<uint64_t>(centerShire) << 24) |
          (static_cast<uint64_t>(neighborShire) << 8) | static_cast<uint64_t>(relativeThreadId));
}

inline void computeStarNeighbors(uint32_t centerShire, uint32_t* neighbors) {
  for (uint32_t idx = 0; idx < kStarNeighborCount; ++idx) {
    neighbors[idx] = gpsdk::star_scratchpad::neighborShire(centerShire, idx);
  }
}

inline volatile uint64_t* getProbeAddress(uint32_t shireId, uint32_t neighborIdx) {
  return reinterpret_cast<volatile uint64_t*>(gpsdk::star_scratchpad::probeAddress(shireId, neighborIdx));
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

  const auto centerShire = gpsdk::device::star_scratchpad::getCenterShireId();
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

  atomic_store_global_64(gpsdk::device::star_scratchpad::successMarkerPtr(),
                         gpsdk::star_scratchpad::kSuccessMarkerValue);

  return 0;
}
