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
#include "gpsdk_nested_scratchpad_example.h"
#include "sync.h"

using gpsdk::examples::nested_scratchpad::KernelArguments;
using gpsdk::examples::nested_scratchpad::Result;

int entryPoint(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint, nullptr);

namespace {

inline volatile uint32_t* inputTilePtr(uint64_t pixelIndex) {
  return gpsdk::device::star_scratchpad::ptr<uint32_t>(
    gpsdk::examples::nested_scratchpad::kInputTileOffset + (pixelIndex * sizeof(uint32_t)));
}

inline volatile uint32_t* outputTilePtr(uint64_t pixelIndex) {
  return gpsdk::device::star_scratchpad::ptr<uint32_t>(
    gpsdk::examples::nested_scratchpad::kOutputTileOffset + (pixelIndex * sizeof(uint32_t)));
}

} // namespace

int entryPoint(KernelArguments* args) {
  et_assert(args != nullptr);
  et_assert(args->resultAddress != 0ULL);
  et_assert(gpsdk::device::star_scratchpad::isAvailable());
  et_assert(gpsdk::device::star_scratchpad::capacity() == (16ULL << 20));

  auto* result = reinterpret_cast<volatile Result*>(args->resultAddress);
  const auto threadId = get_relative_thread_id();
  const auto numThreads = static_cast<uint32_t>(get_num_threads());

  et_assert(threadId >= 0);
  et_assert(numThreads <= gpsdk::examples::nested_scratchpad::kMaxThreadChecksums);

  for (uint64_t pixelIndex = static_cast<uint64_t>(threadId);
       pixelIndex < gpsdk::examples::nested_scratchpad::kTilePixels; pixelIndex += numThreads) {
    const auto x = static_cast<uint32_t>(pixelIndex % gpsdk::examples::nested_scratchpad::kTileWidth);
    const auto y = static_cast<uint32_t>(pixelIndex / gpsdk::examples::nested_scratchpad::kTileWidth);
    atomic_store_global_32(inputTilePtr(pixelIndex), gpsdk::examples::nested_scratchpad::makeInputValue(x, y));
  }

  hart::barrier();

  for (uint64_t pixelIndex = static_cast<uint64_t>(threadId);
       pixelIndex < gpsdk::examples::nested_scratchpad::kTilePixels; pixelIndex += numThreads) {
    const auto x = static_cast<uint32_t>(pixelIndex % gpsdk::examples::nested_scratchpad::kTileWidth);
    const auto y = static_cast<uint32_t>(pixelIndex / gpsdk::examples::nested_scratchpad::kTileWidth);
    const auto center = atomic_load_global_32(inputTilePtr(pixelIndex));

    if ((x == 0U) || (y == 0U) || (x + 1U == gpsdk::examples::nested_scratchpad::kTileWidth) ||
        (y + 1U == gpsdk::examples::nested_scratchpad::kTileHeight)) {
      atomic_store_global_32(outputTilePtr(pixelIndex), center);
      continue;
    }

    const auto left = atomic_load_global_32(inputTilePtr(pixelIndex - 1ULL));
    const auto right = atomic_load_global_32(inputTilePtr(pixelIndex + 1ULL));
    const auto up = atomic_load_global_32(inputTilePtr(pixelIndex - gpsdk::examples::nested_scratchpad::kTileWidth));
    const auto down =
      atomic_load_global_32(inputTilePtr(pixelIndex + gpsdk::examples::nested_scratchpad::kTileWidth));
    atomic_store_global_32(outputTilePtr(pixelIndex), (center * 2U) + left + right + up + down);
  }

  hart::barrier();

  uint64_t checksum = 0ULL;
  for (uint64_t pixelIndex = static_cast<uint64_t>(threadId);
       pixelIndex < gpsdk::examples::nested_scratchpad::kTilePixels; pixelIndex += numThreads) {
    checksum += atomic_load_global_32(outputTilePtr(pixelIndex));
  }
  atomic_store_global_64(&result->threadChecksums[threadId], checksum);

  hart::barrier();

  if (threadId == 0) {
    uint64_t totalChecksum = 0ULL;
    for (uint32_t idx = 0U; idx < numThreads; ++idx) {
      totalChecksum += atomic_load_global_64(&result->threadChecksums[idx]);
    }

    result->magic = gpsdk::examples::nested_scratchpad::kResultMagic;
    result->width = gpsdk::examples::nested_scratchpad::kTileWidth;
    result->height = gpsdk::examples::nested_scratchpad::kTileHeight;
    result->activeThreads = numThreads;
    result->centerShire = gpsdk::device::star_scratchpad::getCenterShireId();
    result->checksum = totalChecksum;
    result->sampleTopLeft = atomic_load_global_32(outputTilePtr(gpsdk::examples::nested_scratchpad::kTileWidth + 1ULL));

    const uint64_t centerIndex =
      (static_cast<uint64_t>(gpsdk::examples::nested_scratchpad::kTileHeight / 2U) *
       gpsdk::examples::nested_scratchpad::kTileWidth) +
      (gpsdk::examples::nested_scratchpad::kTileWidth / 2U);
    result->sampleCenter = atomic_load_global_32(outputTilePtr(centerIndex));

    const uint64_t bottomRightIndex =
      (static_cast<uint64_t>(gpsdk::examples::nested_scratchpad::kTileHeight - 2U) *
       gpsdk::examples::nested_scratchpad::kTileWidth) +
      (gpsdk::examples::nested_scratchpad::kTileWidth - 2U);
    result->sampleBottomRight = atomic_load_global_32(outputTilePtr(bottomRightIndex));
  }

  return 0;
}
