/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#ifndef GPSDK_STAR_SCRATCHPAD_H
#define GPSDK_STAR_SCRATCHPAD_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace gpsdk::star_scratchpad {

constexpr uint32_t kLegacyRows = 4U;
constexpr uint32_t kLegacyCols = 8U;
constexpr uint32_t kLogicalComputeShires = 32U;
constexpr uint32_t kNestedPhysicalRows = 6U;
constexpr uint32_t kNestedPhysicalCols = 8U;
constexpr uint32_t kStarNeighborCount = 4U;
constexpr uint32_t kBlockNeighborCount = 8U;
constexpr uint32_t kNestedRelayCount = 4U;
constexpr uint32_t kNestedLeafCount = 8U;
constexpr uint32_t kInvalidShire = 0xFFFFFFFFU;

constexpr uint64_t kFormat0BaseAddress = 0x80000000ULL;
constexpr uint64_t kShireBytes = 0x280000ULL;

constexpr uint64_t kProbeBaseOffset = kShireBytes - 0x4000ULL;
constexpr uint64_t kProbeNeighborStride = 0x400ULL;

// Reserve 2 MiB in each auxiliary shire to build a stable 8 MiB logical pool while
// leaving guard space at both ends of the 2.5 MiB ETSOC1 scratchpad slice.
constexpr uint64_t kPoolBaseOffset = 0x40000ULL;
constexpr uint64_t kPoolBytesPerAuxShire = 0x200000ULL;
constexpr uint64_t kStarPoolTotalBytes = kStarNeighborCount * kPoolBytesPerAuxShire;
constexpr uint64_t kBlockPoolTotalBytes = kBlockNeighborCount * kPoolBytesPerAuxShire;
constexpr uint64_t kNestedStarPoolTotalBytes = kNestedLeafCount * kPoolBytesPerAuxShire;

constexpr uint64_t kSuccessMarkerOffset = kProbeBaseOffset + 0x2000ULL;
constexpr uint64_t kSuccessMarkerValue = 0x5354415250524F42ULL;

enum class NeighborIndex : uint32_t {
  North = 0U,
  East = 1U,
  South = 2U,
  West = 3U,
};

enum class ClusterLayout : uint32_t {
  Star = 0U,
  Block = 1U,
  NestedStar = 2U,
};

struct PhysicalCoord {
  uint8_t row;
  uint8_t col;
};

struct ClusterSelection {
  uint32_t effectiveCenterShire = kInvalidShire;
  uint64_t computeShireMask = 0ULL;
  uint64_t launchedShireMask = 0ULL;
  bool centerShifted = false;

  constexpr bool valid() const {
    return (effectiveCenterShire != kInvalidShire) && (computeShireMask != 0ULL) && (launchedShireMask != 0ULL);
  }
};

constexpr PhysicalCoord kInvalidCoord = {0xFFU, 0xFFU};

// Nested-star currently uses a host-side physical placement model that extends the existing
// 32 logical compute shires onto a 6x8 NoC patch. The bottom row is only partially exposed
// through the current 32-bit compute-shire mask.
constexpr std::array<PhysicalCoord, kLogicalComputeShires> kNestedLogicalShireCoords = {{
  {0U, 1U}, {0U, 2U}, {0U, 3U}, {0U, 4U}, {0U, 5U}, {0U, 6U},
  {1U, 1U}, {1U, 2U}, {1U, 3U}, {1U, 4U}, {1U, 5U}, {1U, 6U},
  {2U, 1U}, {2U, 2U}, {2U, 3U}, {2U, 4U}, {2U, 5U}, {2U, 6U},
  {3U, 1U}, {3U, 2U}, {3U, 3U}, {3U, 4U}, {3U, 5U}, {3U, 6U},
  {4U, 1U}, {4U, 2U}, {4U, 3U}, {4U, 4U}, {4U, 5U}, {4U, 6U},
  {5U, 3U}, {5U, 4U},
}};

inline constexpr bool isValidLegacyCenterShire(uint32_t centerShire) {
  if (centerShire >= (kLegacyRows * kLegacyCols)) {
    return false;
  }

  const auto row = centerShire / kLegacyCols;
  const auto col = centerShire % kLegacyCols;
  return (row > 0U) && (row + 1U < kLegacyRows) && (col > 0U) && (col + 1U < kLegacyCols);
}

inline constexpr bool isValidNestedCoord(PhysicalCoord coord) {
  return (coord.row < kNestedPhysicalRows) && (coord.col < kNestedPhysicalCols);
}

inline constexpr PhysicalCoord nestedPhysicalCoord(uint32_t shireId) {
  return (shireId < kNestedLogicalShireCoords.size()) ? kNestedLogicalShireCoords[shireId] : kInvalidCoord;
}

inline constexpr uint32_t shireForNestedCoord(uint32_t row, uint32_t col) {
  for (uint32_t shire = 0U; shire < kNestedLogicalShireCoords.size(); ++shire) {
    if ((kNestedLogicalShireCoords[shire].row == row) && (kNestedLogicalShireCoords[shire].col == col)) {
      return shire;
    }
  }
  return kInvalidShire;
}

inline constexpr uint32_t nestedShireAtOffset(uint32_t centerShire, int32_t rowDelta, int32_t colDelta) {
  const auto center = nestedPhysicalCoord(centerShire);
  if (!isValidNestedCoord(center)) {
    return kInvalidShire;
  }

  const auto row = static_cast<int32_t>(center.row) + rowDelta;
  const auto col = static_cast<int32_t>(center.col) + colDelta;
  if ((row < 0) || (col < 0)) {
    return kInvalidShire;
  }
  return shireForNestedCoord(static_cast<uint32_t>(row), static_cast<uint32_t>(col));
}

inline constexpr uint32_t nestedRelayShire(uint32_t centerShire, uint32_t index) {
  switch (index) {
  case 0U:
    return nestedShireAtOffset(centerShire, -1, 0);
  case 1U:
    return nestedShireAtOffset(centerShire, 0, 1);
  case 2U:
    return nestedShireAtOffset(centerShire, 1, 0);
  case 3U:
    return nestedShireAtOffset(centerShire, 0, -1);
  default:
    return kInvalidShire;
  }
}

inline constexpr uint32_t nestedLeafShire(uint32_t centerShire, uint32_t index) {
  switch (index) {
  case 0U:
    return nestedShireAtOffset(centerShire, -2, 0);
  case 1U:
    return nestedShireAtOffset(centerShire, -1, 1);
  case 2U:
    return nestedShireAtOffset(centerShire, 0, 2);
  case 3U:
    return nestedShireAtOffset(centerShire, 1, 1);
  case 4U:
    return nestedShireAtOffset(centerShire, 2, 0);
  case 5U:
    return nestedShireAtOffset(centerShire, 1, -1);
  case 6U:
    return nestedShireAtOffset(centerShire, 0, -2);
  case 7U:
    return nestedShireAtOffset(centerShire, -1, -1);
  default:
    return kInvalidShire;
  }
}

inline constexpr uint64_t clusterShireMaskForCenter(uint32_t centerShire, ClusterLayout layout) {
  if (layout == ClusterLayout::Star) {
    if (!isValidLegacyCenterShire(centerShire)) {
      return 0ULL;
    }

    return (1ULL << centerShire) | (1ULL << (centerShire - 1U)) | (1ULL << (centerShire + 1U)) |
           (1ULL << (centerShire - kLegacyCols)) | (1ULL << (centerShire + kLegacyCols));
  }

  if (layout == ClusterLayout::Block) {
    if (!isValidLegacyCenterShire(centerShire)) {
      return 0ULL;
    }

    return (1ULL << centerShire) | (1ULL << (centerShire - kLegacyCols - 1U)) | (1ULL << (centerShire - kLegacyCols)) |
           (1ULL << (centerShire - kLegacyCols + 1U)) | (1ULL << (centerShire - 1U)) | (1ULL << (centerShire + 1U)) |
           (1ULL << (centerShire + kLegacyCols - 1U)) | (1ULL << (centerShire + kLegacyCols)) |
           (1ULL << (centerShire + kLegacyCols + 1U));
  }

  uint64_t mask = 0ULL;
  const auto centerCoord = nestedPhysicalCoord(centerShire);
  if (!isValidNestedCoord(centerCoord)) {
    return 0ULL;
  }

  mask |= (1ULL << centerShire);
  for (uint32_t relay = 0U; relay < kNestedRelayCount; ++relay) {
    const auto shire = nestedRelayShire(centerShire, relay);
    if (shire == kInvalidShire) {
      return 0ULL;
    }
    mask |= (1ULL << shire);
  }
  for (uint32_t leaf = 0U; leaf < kNestedLeafCount; ++leaf) {
    const auto shire = nestedLeafShire(centerShire, leaf);
    if (shire == kInvalidShire) {
      return 0ULL;
    }
    mask |= (1ULL << shire);
  }
  return mask;
}

inline constexpr bool isValidCenterShire(uint32_t centerShire, ClusterLayout layout = ClusterLayout::Star) {
  return clusterShireMaskForCenter(centerShire, layout) != 0ULL;
}

inline constexpr uint64_t format0Address(uint32_t shireId, uint64_t offset) {
  return (((static_cast<uint64_t>(shireId) << 23) & 0x3F800000ULL) + kFormat0BaseAddress + offset);
}

inline constexpr uint32_t neighborShire(uint32_t centerShire, NeighborIndex index) {
  switch (index) {
  case NeighborIndex::North:
    return centerShire - kLegacyCols;
  case NeighborIndex::East:
    return centerShire + 1U;
  case NeighborIndex::South:
    return centerShire + kLegacyCols;
  case NeighborIndex::West:
    return centerShire - 1U;
  }

  return centerShire;
}

inline constexpr uint32_t neighborShire(uint32_t centerShire, uint32_t index) {
  return neighborShire(centerShire, static_cast<NeighborIndex>(index));
}

inline constexpr uint32_t blockNeighborShire(uint32_t centerShire, uint32_t index) {
  switch (index) {
  case 0U:
    return centerShire - kLegacyCols;
  case 1U:
    return centerShire - kLegacyCols + 1U;
  case 2U:
    return centerShire + 1U;
  case 3U:
    return centerShire + kLegacyCols + 1U;
  case 4U:
    return centerShire + kLegacyCols;
  case 5U:
    return centerShire + kLegacyCols - 1U;
  case 6U:
    return centerShire - 1U;
  case 7U:
    return centerShire - kLegacyCols - 1U;
  default:
    return centerShire;
  }
}

inline constexpr uint32_t nestedLeafPoolShire(uint32_t centerShire, uint32_t index) {
  return nestedLeafShire(centerShire, index);
}

inline constexpr uint64_t probeAddress(uint32_t shireId, uint32_t neighborIndex) {
  return format0Address(shireId, kProbeBaseOffset + (static_cast<uint64_t>(neighborIndex) * kProbeNeighborStride));
}

inline constexpr uint64_t successMarkerAddress(uint32_t shireId) {
  return format0Address(shireId, kSuccessMarkerOffset);
}

inline constexpr uint32_t auxiliaryShireCount(ClusterLayout layout) {
  switch (layout) {
  case ClusterLayout::Star:
    return kStarNeighborCount;
  case ClusterLayout::Block:
    return kBlockNeighborCount;
  case ClusterLayout::NestedStar:
    return kNestedLeafCount;
  }

  return 0U;
}

inline constexpr uint64_t poolCapacity(ClusterLayout layout) {
  switch (layout) {
  case ClusterLayout::Star:
    return kStarPoolTotalBytes;
  case ClusterLayout::Block:
    return kBlockPoolTotalBytes;
  case ClusterLayout::NestedStar:
    return kNestedStarPoolTotalBytes;
  }

  return 0ULL;
}

inline constexpr uint32_t auxiliaryShire(uint32_t centerShire, uint32_t shardIndex, ClusterLayout layout) {
  switch (layout) {
  case ClusterLayout::Star:
    return neighborShire(centerShire, shardIndex);
  case ClusterLayout::Block:
    return blockNeighborShire(centerShire, shardIndex);
  case ClusterLayout::NestedStar:
    return nestedLeafPoolShire(centerShire, shardIndex);
  }

  return centerShire;
}

inline constexpr bool isValidPoolRange(uint64_t logicalOffset, uint64_t sizeBytes, ClusterLayout layout) {
  return (sizeBytes != 0U) && (logicalOffset < poolCapacity(layout)) &&
         (sizeBytes <= (poolCapacity(layout) - logicalOffset));
}

inline constexpr uint32_t poolShardIndex(uint64_t logicalOffset) {
  return static_cast<uint32_t>(logicalOffset / kPoolBytesPerAuxShire);
}

inline constexpr uint64_t poolShardOffset(uint64_t logicalOffset) {
  return kPoolBaseOffset + (logicalOffset % kPoolBytesPerAuxShire);
}

inline constexpr uint64_t poolAddress(uint32_t centerShire, uint64_t logicalOffset, ClusterLayout layout) {
  return format0Address(auxiliaryShire(centerShire, poolShardIndex(logicalOffset), layout),
                        poolShardOffset(logicalOffset));
}

inline constexpr bool isActiveShire(uint64_t activeMask, uint32_t shireId) {
  return (shireId < 64U) && (((activeMask >> shireId) & 0x1ULL) != 0ULL);
}

inline constexpr bool clusterFitsActiveMask(uint32_t centerShire, uint64_t activeMask, ClusterLayout layout) {
  const auto clusterMask = clusterShireMaskForCenter(centerShire, layout);
  return (clusterMask != 0ULL) && isActiveShire(activeMask, centerShire) && ((clusterMask & ~activeMask) == 0ULL);
}

inline constexpr uint32_t manhattanDistance(PhysicalCoord lhs, PhysicalCoord rhs) {
  const auto rowDistance = (lhs.row >= rhs.row) ? (lhs.row - rhs.row) : (rhs.row - lhs.row);
  const auto colDistance = (lhs.col >= rhs.col) ? (lhs.col - rhs.col) : (rhs.col - lhs.col);
  return static_cast<uint32_t>(rowDistance) + static_cast<uint32_t>(colDistance);
}

inline ClusterSelection selectCluster(uint64_t requestedCenterMask, uint64_t activeComputeShireMask, ClusterLayout layout) {
  ClusterSelection selection;

  if (__builtin_popcountll(requestedCenterMask) != 1) {
    return selection;
  }

  const auto requestedCenterShire = static_cast<uint32_t>(__builtin_ctzll(requestedCenterMask));

  if (layout != ClusterLayout::NestedStar) {
    if (!clusterFitsActiveMask(requestedCenterShire, activeComputeShireMask, layout)) {
      return selection;
    }

    selection.effectiveCenterShire = requestedCenterShire;
    selection.computeShireMask = requestedCenterMask;
    selection.launchedShireMask = clusterShireMaskForCenter(requestedCenterShire, layout);
    return selection;
  }

  if (clusterFitsActiveMask(requestedCenterShire, activeComputeShireMask, layout)) {
    selection.effectiveCenterShire = requestedCenterShire;
    selection.computeShireMask = requestedCenterMask;
    selection.launchedShireMask = clusterShireMaskForCenter(requestedCenterShire, layout);
    return selection;
  }

  const auto requestedCoord = nestedPhysicalCoord(requestedCenterShire);
  uint32_t bestCenterShire = kInvalidShire;
  uint32_t bestDistance = 0xFFFFFFFFU;

  for (uint32_t candidate = 0U; candidate < kLogicalComputeShires; ++candidate) {
    if (!clusterFitsActiveMask(candidate, activeComputeShireMask, layout)) {
      continue;
    }

    const auto candidateCoord = nestedPhysicalCoord(candidate);
    const auto candidateDistance =
      isValidNestedCoord(requestedCoord) ? manhattanDistance(requestedCoord, candidateCoord) : 0U;

    if ((bestCenterShire == kInvalidShire) || (candidateDistance < bestDistance) ||
        ((candidateDistance == bestDistance) && (candidate < bestCenterShire))) {
      bestCenterShire = candidate;
      bestDistance = candidateDistance;
    }
  }

  if (bestCenterShire == kInvalidShire) {
    return selection;
  }

  selection.effectiveCenterShire = bestCenterShire;
  selection.computeShireMask = (1ULL << bestCenterShire);
  selection.launchedShireMask = clusterShireMaskForCenter(bestCenterShire, layout);
  selection.centerShifted = (bestCenterShire != requestedCenterShire);
  return selection;
}

} // namespace gpsdk::star_scratchpad

#endif // GPSDK_STAR_SCRATCHPAD_H
