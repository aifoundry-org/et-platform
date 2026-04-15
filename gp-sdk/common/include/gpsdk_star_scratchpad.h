/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#ifndef GPSDK_STAR_SCRATCHPAD_H
#define GPSDK_STAR_SCRATCHPAD_H

#include <cstddef>
#include <cstdint>

namespace gpsdk::star_scratchpad {

constexpr uint32_t kRows = 4U;
constexpr uint32_t kCols = 8U;
constexpr uint32_t kNeighborCount = 4U;

constexpr uint64_t kFormat0BaseAddress = 0x80000000ULL;
constexpr uint64_t kShireBytes = 0x280000ULL;

constexpr uint64_t kProbeBaseOffset = kShireBytes - 0x4000ULL;
constexpr uint64_t kProbeNeighborStride = 0x400ULL;

// Reserve 2 MiB in each auxiliary shire to build a stable 8 MiB logical pool while
// leaving guard space at both ends of the 2.5 MiB ETSOC1 scratchpad slice.
constexpr uint64_t kPoolBaseOffset = 0x40000ULL;
constexpr uint64_t kPoolBytesPerAuxShire = 0x200000ULL;
constexpr uint64_t kPoolTotalBytes = kNeighborCount * kPoolBytesPerAuxShire;

constexpr uint64_t kSuccessMarkerOffset = kProbeBaseOffset + 0x2000ULL;
constexpr uint64_t kSuccessMarkerValue = 0x5354415250524F42ULL;

enum class NeighborIndex : uint32_t {
  North = 0U,
  East = 1U,
  South = 2U,
  West = 3U,
};

inline constexpr bool isValidCenterShire(uint32_t centerShire) {
  if (centerShire >= (kRows * kCols)) {
    return false;
  }

  const auto row = centerShire / kCols;
  const auto col = centerShire % kCols;
  return (row > 0U) && (row + 1U < kRows) && (col > 0U) && (col + 1U < kCols);
}

inline constexpr uint64_t format0Address(uint32_t shireId, uint64_t offset) {
  return (((static_cast<uint64_t>(shireId) << 23) & 0x3F800000ULL) + kFormat0BaseAddress + offset);
}

inline constexpr uint32_t neighborShire(uint32_t centerShire, NeighborIndex index) {
  switch (index) {
  case NeighborIndex::North:
    return centerShire - kCols;
  case NeighborIndex::East:
    return centerShire + 1U;
  case NeighborIndex::South:
    return centerShire + kCols;
  case NeighborIndex::West:
    return centerShire - 1U;
  }

  return centerShire;
}

inline constexpr uint32_t neighborShire(uint32_t centerShire, uint32_t index) {
  return neighborShire(centerShire, static_cast<NeighborIndex>(index));
}

inline constexpr uint64_t probeAddress(uint32_t shireId, uint32_t neighborIndex) {
  return format0Address(shireId, kProbeBaseOffset + (static_cast<uint64_t>(neighborIndex) * kProbeNeighborStride));
}

inline constexpr uint64_t successMarkerAddress(uint32_t shireId) {
  return format0Address(shireId, kSuccessMarkerOffset);
}

inline constexpr bool isValidPoolRange(uint64_t logicalOffset, uint64_t sizeBytes = 1U) {
  return (sizeBytes != 0U) && (logicalOffset < kPoolTotalBytes) && (sizeBytes <= (kPoolTotalBytes - logicalOffset));
}

inline constexpr uint32_t poolShardIndex(uint64_t logicalOffset) {
  return static_cast<uint32_t>(logicalOffset / kPoolBytesPerAuxShire);
}

inline constexpr uint64_t poolShardOffset(uint64_t logicalOffset) {
  return kPoolBaseOffset + (logicalOffset % kPoolBytesPerAuxShire);
}

inline constexpr uint64_t poolAddress(uint32_t centerShire, uint64_t logicalOffset) {
  return format0Address(neighborShire(centerShire, poolShardIndex(logicalOffset)), poolShardOffset(logicalOffset));
}

} // namespace gpsdk::star_scratchpad

#endif // GPSDK_STAR_SCRATCHPAD_H
