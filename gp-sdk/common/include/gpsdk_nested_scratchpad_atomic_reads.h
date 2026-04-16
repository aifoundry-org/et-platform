/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#ifndef GPSDK_NESTED_SCRATCHPAD_ATOMIC_READS_H
#define GPSDK_NESTED_SCRATCHPAD_ATOMIC_READS_H

#include <array>
#include <cstddef>
#include <cstdint>

#include "gpsdk_star_scratchpad.h"

namespace gpsdk::examples::nested_atomic_reads {

constexpr uint64_t kResultMagic = 0x4E535441544F4D31ULL; // "NSTATOM1"
constexpr uint32_t kLeafCount = gpsdk::star_scratchpad::kNestedLeafCount;
constexpr uint32_t kRounds = 512U;
constexpr uint32_t kSlotsPerLeaf = 64U;
constexpr uint64_t kSlotStrideBytes = 64ULL;
constexpr uint32_t kMaxThreadResults = 64U;

struct KernelArguments {
  uint64_t resultAddress = 0ULL;
};

struct alignas(64) Result {
  uint64_t magic = 0ULL;
  uint32_t activeThreads = 0U;
  uint32_t centerShire = 0U;
  uint32_t rounds = 0U;
  uint32_t slotsPerLeaf = 0U;
  uint64_t aggregateChecksum = 0ULL;
  uint64_t aggregateReadCount = 0ULL;
  uint32_t combinedVisitedLeafMask = 0U;
  uint32_t lastActiveThreadId = 0U;
  uint64_t firstThreadChecksum = 0ULL;
  uint64_t lastThreadChecksum = 0ULL;
  uint64_t perLeafReadCounts[kLeafCount] = {};
  uint8_t leafShires[kLeafCount] = {};
  uint8_t reserved[24] = {};
};

inline constexpr uint64_t logicalOffset(uint32_t leafIndex, uint32_t slotIndex) {
  return (static_cast<uint64_t>(leafIndex) * gpsdk::star_scratchpad::kPoolBytesPerAuxShire) +
         (static_cast<uint64_t>(slotIndex) * kSlotStrideBytes);
}

inline constexpr uint64_t makeLeafValue(uint32_t centerShire, uint32_t leafShire, uint32_t slotIndex) {
  return 0x41544F4D00000000ULL | (static_cast<uint64_t>(centerShire) << 24) |
         (static_cast<uint64_t>(leafShire) << 16) | static_cast<uint64_t>(slotIndex);
}

inline constexpr uint64_t checksumStep(uint64_t value, uint32_t threadId, uint32_t round, uint32_t leafIndex,
                                       uint32_t slotIndex) {
  const uint64_t salt = (static_cast<uint64_t>(threadId) << 48) | (static_cast<uint64_t>(round) << 24) |
                        (static_cast<uint64_t>(leafIndex) << 12) | static_cast<uint64_t>(slotIndex);
  return (value ^ salt) * 0x9E3779B185EBCA87ULL;
}

} // namespace gpsdk::examples::nested_atomic_reads

#endif // GPSDK_NESTED_SCRATCHPAD_ATOMIC_READS_H
