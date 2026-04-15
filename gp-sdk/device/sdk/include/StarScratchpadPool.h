/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#ifndef GPSDK_STAR_SCRATCHPAD_POOL_H
#define GPSDK_STAR_SCRATCHPAD_POOL_H

#include <cstddef>
#include <cstdint>

#include "CommonCode.h"
#include "gpsdk_star_scratchpad.h"

namespace gpsdk::device::star_scratchpad {

inline gpsdk::star_scratchpad::ClusterLayout getLayout() {
  if (isScratchpadNestedStarClusterEnabled()) {
    return gpsdk::star_scratchpad::ClusterLayout::NestedStar;
  }
  return isScratchpadBlockClusterEnabled() ? gpsdk::star_scratchpad::ClusterLayout::Block
                                           : gpsdk::star_scratchpad::ClusterLayout::Star;
}

inline uint32_t getCenterShireId() {
  const auto computeShireMask = getComputeShireMask();
  et_assert(__builtin_popcountll(computeShireMask) == 1);

  const auto centerShire = static_cast<uint32_t>(__builtin_ctzll(computeShireMask));
  et_assert(gpsdk::star_scratchpad::isValidCenterShire(centerShire, getLayout()));
  return centerShire;
}

inline bool isAvailable() {
  const auto computeShireMask = getComputeShireMask();
  return (isScratchpadStarClusterEnabled() || isScratchpadBlockClusterEnabled() || isScratchpadNestedStarClusterEnabled()) &&
         (__builtin_popcountll(computeShireMask) == 1) &&
         gpsdk::star_scratchpad::isValidCenterShire(static_cast<uint32_t>(__builtin_ctzll(computeShireMask)),
                                                    getLayout());
}

inline uint64_t capacity() {
  return gpsdk::star_scratchpad::poolCapacity(getLayout());
}

inline uint64_t address(uint64_t logicalOffset, uint64_t sizeBytes = 1U) {
  et_assert(isAvailable());
  et_assert(gpsdk::star_scratchpad::isValidPoolRange(logicalOffset, sizeBytes, getLayout()));
  return gpsdk::star_scratchpad::poolAddress(getCenterShireId(), logicalOffset, getLayout());
}

template <typename T = std::byte>
inline volatile T* ptr(uint64_t logicalOffset = 0U) {
  return reinterpret_cast<volatile T*>(address(logicalOffset, sizeof(T)));
}

inline volatile uint64_t* successMarkerPtr() {
  return reinterpret_cast<volatile uint64_t*>(
    gpsdk::star_scratchpad::successMarkerAddress(getCenterShireId()));
}

} // namespace gpsdk::device::star_scratchpad

#endif // GPSDK_STAR_SCRATCHPAD_POOL_H
