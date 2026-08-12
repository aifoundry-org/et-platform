/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#ifndef GPSDK_TOPOLOGY_PROBE_H
#define GPSDK_TOPOLOGY_PROBE_H

#include <cstdint>

#include "gpsdk_star_scratchpad.h"

namespace gpsdk::topology_probe {

constexpr uint64_t kResultsMagic = 0x544F504F4C415430ULL; // "TOPOLAT0"
constexpr uint32_t kVisibleComputeShires = 32U;
constexpr uint32_t kSampleBatches = 8U;
constexpr uint32_t kBatchIterations = 128U;

constexpr uint64_t kRemoteProbeOffset = 0x20000ULL;
constexpr uint64_t kResultsOffset = gpsdk::star_scratchpad::kShireBytes - 0x10000ULL;

struct ProbeArguments {
  uint64_t targetShireMask = 0xFFFFFFFFULL;
  uint64_t resultsAddress = 0ULL;
};

struct alignas(64) ShireLatencyResults {
  uint64_t magic = kResultsMagic;
  uint64_t targetShireMask = 0ULL;
  uint32_t centerShire = 0U;
  uint32_t sampleBatches = 0U;
  uint32_t batchIterations = 0U;
  uint32_t reserved0 = 0U;
  uint64_t loadBestCycles[kVisibleComputeShires] = {};
  uint64_t storeBestCycles[kVisibleComputeShires] = {};
  uint64_t loadChecksum = 0ULL;
  uint64_t storeChecksum = 0ULL;
};

static_assert((sizeof(ShireLatencyResults) % 64U) == 0U, "ShireLatencyResults should be cacheline aligned");

inline constexpr uint64_t remoteProbeAddress(uint32_t shireId) {
  return gpsdk::star_scratchpad::format0Address(shireId, kRemoteProbeOffset);
}

} // namespace gpsdk::topology_probe

#endif // GPSDK_TOPOLOGY_PROBE_H
