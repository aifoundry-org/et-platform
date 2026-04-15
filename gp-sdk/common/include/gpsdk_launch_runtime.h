/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#ifndef GPSDK_LAUNCH_RUNTIME_H
#define GPSDK_LAUNCH_RUNTIME_H

#include <cstdint>

namespace gpsdk::launch {

constexpr uint64_t kRuntimeArgsMagic = 0x475053444B534849ULL;
constexpr uint16_t kRuntimeArgsVersion = 1U;
constexpr uint8_t kInvalidShireId = 0xFFU;

constexpr uint32_t kMinionsPerShire = 32U;
constexpr uint32_t kNeighborhoodsPerShire = 4U;
constexpr uint32_t kMinionsPerNeighborhood = kMinionsPerShire / kNeighborhoodsPerShire;
constexpr uint32_t kAllMinionsMask = 0xFFFFFFFFU;
constexpr uint32_t kMaxScratchpadRelayShires = 4U;
constexpr uint32_t kMaxScratchpadAuxiliaryShires = 8U;

enum LaunchFlags : uint32_t {
  kLaunchFlagSingleNeighborhoodPerShire = 1U << 0,
  kLaunchFlagScratchpadStarCluster = 1U << 1,
  kLaunchFlagScratchpadBlockCluster = 1U << 2,
  kLaunchFlagScratchpadNestedStarCluster = 1U << 3,
  kLaunchFlagErbiumSim = 1U << 4,
};

struct alignas(64) RuntimeArgsHeader {
  uint64_t magic = kRuntimeArgsMagic;
  uint16_t version = kRuntimeArgsVersion;
  uint16_t headerSize = 64U;
  uint32_t flags = 0U;
  uint32_t payloadSize = 0U;
  uint8_t activeNeighborhood = 0U;
  uint8_t effectiveCenterShire = kInvalidShireId;
  uint8_t scratchpadRelayCount = 0U;
  uint8_t scratchpadAuxiliaryCount = 0U;
  uint64_t computeShireMask = 0ULL;
  uint8_t scratchpadRelayShires[kMaxScratchpadRelayShires] = {
    kInvalidShireId, kInvalidShireId, kInvalidShireId, kInvalidShireId};
  uint8_t scratchpadAuxiliaryShires[kMaxScratchpadAuxiliaryShires] = {
    kInvalidShireId,
    kInvalidShireId,
    kInvalidShireId,
    kInvalidShireId,
    kInvalidShireId,
    kInvalidShireId,
    kInvalidShireId,
    kInvalidShireId,
  };
  uint8_t reserved[16] = {};
};

static_assert(sizeof(RuntimeArgsHeader) == 64U, "RuntimeArgsHeader must occupy exactly one cache line");

inline constexpr bool isValidNeighborhood(uint32_t neighborhood) {
  return neighborhood < kNeighborhoodsPerShire;
}

inline constexpr uint32_t minionMaskForNeighborhood(uint32_t neighborhood) {
  return ((1U << kMinionsPerNeighborhood) - 1U) << (neighborhood * kMinionsPerNeighborhood);
}

inline constexpr uint32_t minionsPerShire(uint32_t flags) {
  return (flags & kLaunchFlagSingleNeighborhoodPerShire) ? kMinionsPerNeighborhood : kMinionsPerShire;
}

} // namespace gpsdk::launch

#endif // GPSDK_LAUNCH_RUNTIME_H
