/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <cstdint>

#include <etsoc/common/utils.h>
#include <etsoc/isa/atomic.h>

#include "CommonCode.h"
#include "StarScratchpadPool.h"
#include "entryPoint.h"
#include "gpsdk_nested_scratchpad_atomic_reads.h"
#include "sync.h"

using gpsdk::examples::nested_atomic_reads::KernelArguments;
using gpsdk::examples::nested_atomic_reads::Result;

int entryPoint(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint, nullptr);

namespace {

constexpr uint64_t kReductionBaseOffset = 0x10000ULL;
constexpr uint64_t kReductionStrideBytes = 64ULL;
constexpr uint64_t kVisitedBaseOffset = kReductionBaseOffset + 0x1000ULL;
constexpr uint64_t kReadCountBaseOffset = kReductionBaseOffset + 0x2000ULL;

inline volatile uint64_t* leafSlotPtr(uint32_t leafIndex, uint32_t slotIndex) {
  return gpsdk::device::star_scratchpad::ptr<uint64_t>(
    gpsdk::examples::nested_atomic_reads::logicalOffset(leafIndex, slotIndex));
}

inline volatile uint64_t* checksumSlotPtr(uint32_t centerShire, uint32_t threadId) {
  return reinterpret_cast<volatile uint64_t*>(gpsdk::star_scratchpad::format0Address(
    centerShire, kReductionBaseOffset + (static_cast<uint64_t>(threadId) * kReductionStrideBytes)));
}

inline volatile uint32_t* visitedMaskSlotPtr(uint32_t centerShire, uint32_t threadId) {
  return reinterpret_cast<volatile uint32_t*>(gpsdk::star_scratchpad::format0Address(
    centerShire, kVisitedBaseOffset + (static_cast<uint64_t>(threadId) * kReductionStrideBytes)));
}

inline volatile uint32_t* readCountSlotPtr(uint32_t centerShire, uint32_t threadId) {
  return reinterpret_cast<volatile uint32_t*>(gpsdk::star_scratchpad::format0Address(
    centerShire, kReadCountBaseOffset + (static_cast<uint64_t>(threadId) * kReductionStrideBytes)));
}

} // namespace

int entryPoint(KernelArguments* args) {
  et_assert(args != nullptr);
  et_assert(args->resultAddress != 0ULL);
  et_assert(gpsdk::device::star_scratchpad::isAvailable());
  et_assert(isScratchpadNestedStarClusterEnabled());
  et_assert(gpsdk::device::star_scratchpad::capacity() == (16ULL << 20));

  auto* result = reinterpret_cast<volatile Result*>(args->resultAddress);
  const auto centerShire = gpsdk::device::star_scratchpad::getCenterShireId();
  const auto threadId = static_cast<uint32_t>(get_relative_thread_id());
  const auto numThreads = static_cast<uint32_t>(get_num_threads());

  et_assert(numThreads > 0U);
  et_assert(numThreads <= gpsdk::examples::nested_atomic_reads::kMaxThreadResults);
  et_assert(getScratchpadAuxiliaryCount() == gpsdk::examples::nested_atomic_reads::kLeafCount);

  if (threadId == 0U) {
    for (uint32_t leaf = 0U; leaf < gpsdk::examples::nested_atomic_reads::kLeafCount; ++leaf) {
      result->leafShires[leaf] = static_cast<uint8_t>(getScratchpadAuxiliaryShire(leaf));
      result->perLeafReadCounts[leaf] = 0ULL;
    }
    for (uint32_t idx = 0U; idx < numThreads; ++idx) {
      atomic_store_global_64(checksumSlotPtr(centerShire, idx), 0ULL);
      atomic_store_global_32(visitedMaskSlotPtr(centerShire, idx), 0U);
      atomic_store_global_32(readCountSlotPtr(centerShire, idx), 0U);
    }
    result->magic = 0ULL;
    result->activeThreads = numThreads;
    result->centerShire = centerShire;
    result->rounds = gpsdk::examples::nested_atomic_reads::kRounds;
    result->slotsPerLeaf = gpsdk::examples::nested_atomic_reads::kSlotsPerLeaf;
    result->aggregateChecksum = 0ULL;
    result->aggregateReadCount = 0ULL;
    result->combinedVisitedLeafMask = 0U;
    result->lastActiveThreadId = 0U;
    result->firstThreadChecksum = 0ULL;
    result->lastThreadChecksum = 0ULL;
  }

  if (threadId == 0U) {
    for (uint32_t slot = 0U; slot < gpsdk::examples::nested_atomic_reads::kSlotsPerLeaf; ++slot) {
      for (uint32_t leaf = 0U; leaf < gpsdk::examples::nested_atomic_reads::kLeafCount; ++leaf) {
        const auto leafShire = static_cast<uint32_t>(getScratchpadAuxiliaryShire(leaf));
        atomic_store_global_64(
          leafSlotPtr(leaf, slot),
          gpsdk::examples::nested_atomic_reads::makeLeafValue(centerShire, leafShire, slot));
      }
    }
    asm volatile("fence\n" ::: "memory");
  }

  hart::barrier();

  uint64_t checksum = 0ULL;
  uint32_t visitedLeafMask = 0U;
  uint32_t readCount = 0U;

  for (uint32_t round = 0U; round < gpsdk::examples::nested_atomic_reads::kRounds; ++round) {
    const uint32_t leafIndex = (threadId + round) % gpsdk::examples::nested_atomic_reads::kLeafCount;
    const uint32_t slotIndex =
      (threadId + (round * numThreads)) % gpsdk::examples::nested_atomic_reads::kSlotsPerLeaf;
    const auto value = atomic_load_global_64(leafSlotPtr(leafIndex, slotIndex));

    checksum += gpsdk::examples::nested_atomic_reads::checksumStep(value, threadId, round, leafIndex, slotIndex);
    visitedLeafMask |= (1U << leafIndex);
    ++readCount;
  }

  atomic_store_global_64(checksumSlotPtr(centerShire, threadId), checksum);
  atomic_store_global_32(visitedMaskSlotPtr(centerShire, threadId), visitedLeafMask);
  atomic_store_global_32(readCountSlotPtr(centerShire, threadId), readCount);
  asm volatile("fence\n" ::: "memory");

  hart::barrier();

  if (threadId == 0U) {
    uint64_t aggregateChecksum = 0ULL;
    uint64_t aggregateReadCount = 0ULL;
    uint32_t combinedVisitedLeafMask = 0U;
    for (uint32_t idx = 0U; idx < numThreads; ++idx) {
      aggregateChecksum += atomic_load_global_64(checksumSlotPtr(centerShire, idx));
      aggregateReadCount += atomic_load_global_32(readCountSlotPtr(centerShire, idx));
      combinedVisitedLeafMask |= atomic_load_global_32(visitedMaskSlotPtr(centerShire, idx));
    }

    for (uint32_t round = 0U; round < gpsdk::examples::nested_atomic_reads::kRounds; ++round) {
      for (uint32_t idx = 0U; idx < numThreads; ++idx) {
        const uint32_t leafIndex = (idx + round) % gpsdk::examples::nested_atomic_reads::kLeafCount;
        ++result->perLeafReadCounts[leafIndex];
      }
    }

    result->aggregateChecksum = aggregateChecksum;
    result->aggregateReadCount = aggregateReadCount;
    result->combinedVisitedLeafMask = combinedVisitedLeafMask;
    result->lastActiveThreadId = numThreads - 1U;
    result->firstThreadChecksum = atomic_load_global_64(checksumSlotPtr(centerShire, 0U));
    result->lastThreadChecksum = atomic_load_global_64(checksumSlotPtr(centerShire, numThreads - 1U));
    result->magic = gpsdk::examples::nested_atomic_reads::kResultMagic;
  }

  return 0;
}
