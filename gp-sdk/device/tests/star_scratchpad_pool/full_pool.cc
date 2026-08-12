/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <etsoc/common/utils.h>
#include <etsoc/isa/atomic.h>

#include "CommonCode.h"
#include "StarScratchpadPool.h"
#include "entryPoint.h"
#include "gpsdk_star_scratchpad.h"

class KernelArguments;
int entryPoint(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint, nullptr);

namespace {

constexpr uint64_t kLineStride = 64U;

inline uint64_t makePoolValue(uint32_t centerShire, uint64_t logicalOffset) {
  return 0x5354415200000000ULL | (static_cast<uint64_t>(centerShire) << 24) | logicalOffset;
}

} // namespace

int entryPoint([[maybe_unused]] KernelArguments* args) {
  if (get_relative_thread_id() != 0) {
    return 0;
  }

  et_assert(gpsdk::device::star_scratchpad::isAvailable());
  if (isScratchpadBlockClusterEnabled() || isScratchpadNestedStarClusterEnabled()) {
    et_assert(gpsdk::device::star_scratchpad::capacity() == (16ULL << 20));
  } else {
    et_assert(isScratchpadStarClusterEnabled());
    et_assert(gpsdk::device::star_scratchpad::capacity() == (8ULL << 20));
  }

  const auto centerShire = gpsdk::device::star_scratchpad::getCenterShireId();

  for (uint64_t logicalOffset = 0; logicalOffset < gpsdk::device::star_scratchpad::capacity();
       logicalOffset += kLineStride) {
    atomic_store_global_64(gpsdk::device::star_scratchpad::ptr<uint64_t>(logicalOffset),
                           makePoolValue(centerShire, logicalOffset));
  }

  asm volatile("fence\n" ::: "memory");

  for (uint64_t logicalOffset = 0; logicalOffset < gpsdk::device::star_scratchpad::capacity();
       logicalOffset += kLineStride) {
    const auto value = atomic_load_global_64(gpsdk::device::star_scratchpad::ptr<uint64_t>(logicalOffset));
    et_assert(value == makePoolValue(centerShire, logicalOffset));
  }

  atomic_store_global_64(gpsdk::device::star_scratchpad::successMarkerPtr(),
                         gpsdk::star_scratchpad::kSuccessMarkerValue);
  return 0;
}
