/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <cstdint>

#include <etsoc/common/utils.h>
#include <etsoc/isa/atomic.h>

#include "CommonCode.h"
#include "entryPoint.h"
#include "gpsdk_topology_probe.h"

using gpsdk::topology_probe::ProbeArguments;
using gpsdk::topology_probe::ShireLatencyResults;

int entryPoint(ProbeArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint, nullptr);

namespace {

inline uint32_t getCenterShire() {
  const auto shireMask = getKernelShireMask();
  et_assert(__builtin_popcountll(shireMask) == 1);
  return static_cast<uint32_t>(__builtin_ctzll(shireMask));
}

inline uint64_t timeLoadCycles(volatile uint64_t* ptr, uint64_t* checksum) {
  uint64_t bestCycles = ~0ULL;
  uint64_t bestValue = 0ULL;

  for (uint32_t batch = 0U; batch < gpsdk::topology_probe::kSampleBatches; ++batch) {
    asm volatile("fence\n" ::: "memory");
    const auto start = et_get_timestamp();
    uint64_t localChecksum = 0ULL;
    for (uint32_t iter = 0U; iter < gpsdk::topology_probe::kBatchIterations; ++iter) {
      localChecksum ^= atomic_load_global_64(ptr);
    }
    const auto elapsed = et_get_delta_timestamp(start);
    if (elapsed < bestCycles) {
      bestCycles = elapsed;
      bestValue = localChecksum;
    }
  }

  *checksum ^= bestValue;
  return bestCycles / gpsdk::topology_probe::kBatchIterations;
}

inline uint64_t timeStoreCycles(volatile uint64_t* ptr, uint64_t baseValue, uint64_t* checksum) {
  uint64_t bestCycles = ~0ULL;
  uint64_t bestValue = baseValue;

  for (uint32_t batch = 0U; batch < gpsdk::topology_probe::kSampleBatches; ++batch) {
    asm volatile("fence\n" ::: "memory");
    const auto start = et_get_timestamp();
    uint64_t lastValue = baseValue;
    for (uint32_t iter = 0U; iter < gpsdk::topology_probe::kBatchIterations; ++iter) {
      lastValue = baseValue + iter;
      atomic_store_global_64(ptr, lastValue);
    }
    asm volatile("fence\n" ::: "memory");
    const auto elapsed = et_get_delta_timestamp(start);
    if (elapsed < bestCycles) {
      bestCycles = elapsed;
      bestValue = lastValue;
    }
  }

  *checksum ^= bestValue;
  return bestCycles / gpsdk::topology_probe::kBatchIterations;
}

} // namespace

int entryPoint(ProbeArguments* args) {
  if (get_relative_thread_id() != 0) {
    return 0;
  }

  const auto centerShire = getCenterShire();
  const auto targetMask = (args != nullptr) ? args->targetShireMask : 0xFFFFFFFFULL;
  et_assert(args != nullptr);
  et_assert(args->resultsAddress != 0ULL);
  auto* results = reinterpret_cast<volatile ShireLatencyResults*>(args->resultsAddress);
  uint64_t loadChecksum = 0ULL;
  uint64_t storeChecksum = 0ULL;

  results->magic = gpsdk::topology_probe::kResultsMagic;
  results->targetShireMask = targetMask;
  results->centerShire = centerShire;
  results->sampleBatches = gpsdk::topology_probe::kSampleBatches;
  results->batchIterations = gpsdk::topology_probe::kBatchIterations;
  results->reserved0 = 0U;
  results->loadChecksum = 0ULL;
  results->storeChecksum = 0ULL;

  for (uint32_t targetShire = 0U; targetShire < gpsdk::topology_probe::kVisibleComputeShires; ++targetShire) {
    if (((targetMask >> targetShire) & 0x1ULL) == 0ULL) {
      results->loadBestCycles[targetShire] = 0ULL;
      results->storeBestCycles[targetShire] = 0ULL;
      continue;
    }

    auto* remotePtr = reinterpret_cast<volatile uint64_t*>(gpsdk::topology_probe::remoteProbeAddress(targetShire));
    const uint64_t seedValue =
      0x4C41545000000000ULL | (static_cast<uint64_t>(centerShire) << 16) | static_cast<uint64_t>(targetShire);

    atomic_store_global_64(remotePtr, seedValue);
    asm volatile("fence\n" ::: "memory");

    results->loadBestCycles[targetShire] = timeLoadCycles(remotePtr, &loadChecksum);
    results->storeBestCycles[targetShire] = timeStoreCycles(remotePtr, seedValue + 0x1000ULL, &storeChecksum);
  }

  results->loadChecksum = loadChecksum;
  results->storeChecksum = storeChecksum;
  asm volatile("fence\n" ::: "memory");
  return 0;
}
