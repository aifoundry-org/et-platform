/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#ifndef GPSDK_NESTED_SCRATCHPAD_EXAMPLE_H
#define GPSDK_NESTED_SCRATCHPAD_EXAMPLE_H

#include <cstddef>
#include <cstdint>

namespace gpsdk::examples::nested_scratchpad {

constexpr uint64_t kResultMagic = 0x4E535444454D4F31ULL; // "NSTDEMO1"
constexpr uint32_t kTileWidth = 2048U;
constexpr uint32_t kTileHeight = 1023U;
constexpr uint64_t kTilePixels = static_cast<uint64_t>(kTileWidth) * static_cast<uint64_t>(kTileHeight);
constexpr uint64_t kTileBytes = kTilePixels * sizeof(uint32_t);
constexpr uint64_t kInputTileOffset = 0ULL;
constexpr uint64_t kOutputTileOffset = 8ULL << 20;
constexpr uint32_t kMaxThreadChecksums = 64U;

struct KernelArguments {
  uint64_t resultAddress = 0ULL;
};

struct alignas(64) Result {
  uint64_t magic = 0ULL;
  uint32_t width = 0U;
  uint32_t height = 0U;
  uint32_t activeThreads = 0U;
  uint32_t centerShire = 0U;
  uint64_t checksum = 0ULL;
  uint32_t sampleTopLeft = 0U;
  uint32_t sampleCenter = 0U;
  uint32_t sampleBottomRight = 0U;
  uint64_t threadChecksums[kMaxThreadChecksums] = {};
};

inline constexpr uint32_t makeInputValue(uint32_t x, uint32_t y) {
  return (x * 2654435761U) ^ (y * 2246822519U) ^ 0x13579BDFU;
}

inline constexpr uint32_t linearStencilValue(uint32_t x, uint32_t y) {
  const auto center = makeInputValue(x, y);
  if ((x == 0U) || (y == 0U) || (x + 1U == kTileWidth) || (y + 1U == kTileHeight)) {
    return center;
  }

  return (center * 2U) + makeInputValue(x - 1U, y) + makeInputValue(x + 1U, y) + makeInputValue(x, y - 1U) +
         makeInputValue(x, y + 1U);
}

} // namespace gpsdk::examples::nested_scratchpad

#endif // GPSDK_NESTED_SCRATCHPAD_EXAMPLE_H
