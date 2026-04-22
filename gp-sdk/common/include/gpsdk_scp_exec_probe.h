/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#ifndef GPSDK_SCP_EXEC_PROBE_H
#define GPSDK_SCP_EXEC_PROBE_H

#include <cstdint>

namespace gpsdk::examples::scp_exec_probe {

constexpr uint64_t kResultMagic = 0x5343504558454331ULL; // "SCPEXEC1"

struct KernelArguments {
  uint64_t resultAddress = 0ULL;
  uint64_t expectedEntryAddress = 0ULL;
};

struct alignas(64) Result {
  uint64_t magic = 0ULL;
  uint32_t started = 0U;
  uint32_t completed = 0U;
  uint32_t centerShire = 0U;
  uint32_t threadId = 0U;
  uint64_t expectedEntryAddress = 0ULL;
  uint64_t observedEntryAddress = 0ULL;
};

} // namespace gpsdk::examples::scp_exec_probe

#endif // GPSDK_SCP_EXEC_PROBE_H
