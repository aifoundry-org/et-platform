/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#ifndef GPSDK_ERBIUM_SIM_ACCESS_VIOLATION_H
#define GPSDK_ERBIUM_SIM_ACCESS_VIOLATION_H

#include <cstdint>

namespace gpsdk::examples::erbium_sim_access_violation {

constexpr uint64_t kResultMagic = 0x45524256494F3031ULL; // "ERBVIO01"

enum class AccessMode : uint32_t {
  AtomicRead = 0U,
  GlobalMemcpy = 1U,
  TensorLoad = 2U,
};

struct KernelArguments {
  uint64_t resultAddress = 0ULL;
  uint32_t targetShire = 0U;
  uint32_t accessMode = 0U;
};

struct alignas(64) Result {
  uint64_t magic = 0ULL;
  uint32_t started = 0U;
  uint32_t completed = 0U;
  uint32_t targetShire = 0U;
  uint32_t accessMode = 0U;
  uint32_t centerShire = 0U;
  uint32_t observedWord = 0U;
};

inline constexpr const char* accessModeName(AccessMode mode) {
  switch (mode) {
  case AccessMode::AtomicRead:
    return "atomic_read";
  case AccessMode::GlobalMemcpy:
    return "global_memcpy";
  case AccessMode::TensorLoad:
    return "tensor_load";
  }
  return "unknown";
}

} // namespace gpsdk::examples::erbium_sim_access_violation

#endif // GPSDK_ERBIUM_SIM_ACCESS_VIOLATION_H
