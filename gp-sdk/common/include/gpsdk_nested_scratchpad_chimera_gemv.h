/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#ifndef GPSDK_NESTED_SCRATCHPAD_CHIMERA_GEMV_H
#define GPSDK_NESTED_SCRATCHPAD_CHIMERA_GEMV_H

#include <cstddef>
#include <cstdint>

namespace gpsdk::examples::chimera_gemv {

constexpr uint64_t kResultMagic = 0x434847454D563031ULL; // "CHGEMV01"
constexpr uint32_t kRows = 2048U;
constexpr uint32_t kCols = 1024U;
constexpr uint32_t kTensorProbeRows = 14U;
constexpr uint32_t kTensorProbeCols = 16U;
constexpr uint32_t kTensorProbeK = 16U;
constexpr uint64_t kMatrixBytes = static_cast<uint64_t>(kRows) * static_cast<uint64_t>(kCols) * sizeof(float);
constexpr uint64_t kVectorOffset = kMatrixBytes;
constexpr uint64_t kVectorBytes = static_cast<uint64_t>(kCols) * sizeof(float);
constexpr uint64_t kInitialOutputOffset = kVectorOffset + 0x2000ULL;
constexpr uint64_t kOutputOffset = kInitialOutputOffset + 0x2000ULL;
constexpr uint64_t kOutputBytes = static_cast<uint64_t>(kRows) * sizeof(float);
constexpr float kAlpha = 1.25f;
constexpr float kBeta = -0.5f;

struct KernelArguments {
  uint64_t resultAddress = 0ULL;
};

struct alignas(64) Result {
  alignas(64) float outputs[kRows] = {};
  alignas(64) float tensorProbe[kTensorProbeRows * kTensorProbeCols] = {};
  alignas(64) float tensorProbeA[kTensorProbeRows * kTensorProbeK] = {};
  alignas(64) float tensorProbeB[kTensorProbeK * kTensorProbeCols] = {};
  uint64_t magic = 0ULL;
  uint32_t rows = 0U;
  uint32_t cols = 0U;
  uint32_t activeThreads = 0U;
  uint32_t centerShire = 0U;
  uint32_t tensorProbeRows = 0U;
  uint32_t tensorProbeCols = 0U;
};

inline constexpr float makeMatrixValue(uint32_t row, uint32_t col) {
  const int term = static_cast<int>((row * 13U + col * 7U + 5U) % 97U) - 48;
  return static_cast<float>(term) * 0.015625f;
}

inline constexpr float makeVectorValue(uint32_t col) {
  const int term = static_cast<int>((col * 11U + 3U) % 29U) - 14;
  return static_cast<float>(term) * 0.0625f;
}

inline constexpr float makeInitialOutputValue(uint32_t row) {
  const int term = static_cast<int>((row * 17U + 9U) % 31U) - 15;
  return static_cast<float>(term) * 0.125f;
}

} // namespace gpsdk::examples::chimera_gemv

#endif // GPSDK_NESTED_SCRATCHPAD_CHIMERA_GEMV_H
