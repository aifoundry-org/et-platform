/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <cstdint>

#include <etsoc/common/utils.h>
#include <etsoc/isa/tensors.h>

#include "CommonCode.h"
#include "StarScratchpadPool.h"
#include "entryPoint.h"
#include "gpsdk_nested_scratchpad_chimera_gemv.h"

using gpsdk::examples::chimera_gemv::KernelArguments;
using gpsdk::examples::chimera_gemv::Result;

int entryPoint(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint, nullptr);

namespace {

constexpr uint32_t kPackedLanes = 8U;
constexpr uint32_t kTensorTileRows = gpsdk::examples::chimera_gemv::kTensorProbeRows;
constexpr uint32_t kTensorTileCols = gpsdk::examples::chimera_gemv::kTensorProbeCols;
constexpr uint32_t kTensorTileK = gpsdk::examples::chimera_gemv::kTensorProbeK;
constexpr uint64_t kTensorStrideBytes = 64ULL;
constexpr uint64_t kTensorOpcodeFp32 = 0ULL;

alignas(64) float gTensorProbeA[kTensorTileRows * kTensorTileK];
alignas(64) float gTensorProbeB[kTensorTileK * kTensorTileCols];
alignas(64) float gTensorProbeC[kTensorTileRows * kTensorTileCols];

inline volatile float* matrixPtr(uint32_t row, uint32_t col) {
  return gpsdk::device::star_scratchpad::ptr<float>(
    static_cast<uint64_t>(row) * gpsdk::examples::chimera_gemv::kCols * sizeof(float) +
    static_cast<uint64_t>(col) * sizeof(float));
}

inline volatile float* vectorPtr(uint32_t col) {
  return gpsdk::device::star_scratchpad::ptr<float>(
    gpsdk::examples::chimera_gemv::kVectorOffset + static_cast<uint64_t>(col) * sizeof(float));
}

inline volatile float* initialOutputPtr(uint32_t row) {
  return gpsdk::device::star_scratchpad::ptr<float>(
    gpsdk::examples::chimera_gemv::kInitialOutputOffset + static_cast<uint64_t>(row) * sizeof(float));
}

inline volatile float* outputPtr(uint32_t row) {
  return gpsdk::device::star_scratchpad::ptr<float>(
    gpsdk::examples::chimera_gemv::kOutputOffset + static_cast<uint64_t>(row) * sizeof(float));
}

inline void setFullMask() {
  mask_set(0, 0xff);
}

inline float loadGlobalFloat(volatile const float* src) {
  const uint32_t bits = atomic_load_global_32(reinterpret_cast<volatile const uint32_t*>(src));
  float value = 0.0f;
  __builtin_memcpy(&value, &bits, sizeof(bits));
  return value;
}

inline void storeGlobalFloat(volatile float* dst, float value) {
  uint32_t bits = 0U;
  __builtin_memcpy(&bits, &value, sizeof(bits));
  atomic_store_global_32(reinterpret_cast<volatile uint32_t*>(dst), bits);
}

inline __attribute__((always_inline)) float broadcastPackedFloat(float value) {
  uint32_t bits = 0U;
  __builtin_memcpy(&bits, &value, sizeof(bits));

  float packedValue;
  __asm__ __volatile__("fbcx.ps %[packedValue], %[bits]\n"
                       : [packedValue] "=&f"(packedValue)
                       : [bits] "r"(bits));
  return packedValue;
}

inline __attribute__((always_inline)) float packedLoad8(const float* src) {
  float value;
  __asm__ __volatile__("flw.ps %[value], 0(%[src])\n"
                       : [value] "=&f"(value)
                       : [src] "r"(src)
                       : "memory");
  return value;
}

inline __attribute__((always_inline)) float packedMul(float lhs, float rhs) {
  float value;
  __asm__ __volatile__("fmul.ps %[value], %[lhs], %[rhs]\n"
                       : [value] "=&f"(value)
                       : [lhs] "f"(lhs), [rhs] "f"(rhs));
  return value;
}

inline __attribute__((always_inline)) float packedAdd(float lhs, float rhs) {
  float value;
  __asm__ __volatile__("fadd.ps %[value], %[lhs], %[rhs]\n"
                       : [value] "=&f"(value)
                       : [lhs] "f"(lhs), [rhs] "f"(rhs));
  return value;
}

inline __attribute__((always_inline)) float packedHorizontalSum(float packedValue) {
  alignas(32) float lanes[kPackedLanes];
  __asm__ __volatile__("fsw.ps %[value], 0(%[dst])\n"
                       :
                       : [dst] "r"(lanes), [value] "f"(packedValue)
                       : "memory");

  float result = 0.0f;
  for (uint32_t lane = 0U; lane < kPackedLanes; ++lane) {
    result += lanes[lane];
  }
  return result;
}

void drainCoalescingBuffers() {
  constexpr uint32_t l2CacheBanks = 4U;

  for (uint32_t bank = 0U; bank < l2CacheBanks; ++bank) {
    volatile uint64_t* control = reinterpret_cast<uint64_t*>(ESR_CACHE(SHIRE_OWN, bank, SC_IDX_COP_SM_CTL_USER));
    uint64_t state = 0ULL;
    do {
      state = (*control >> 24) & 0xffULL;
    } while (state != 4ULL);
  }

  for (uint32_t bank = 0U; bank < l2CacheBanks; ++bank) {
    volatile uint64_t* control = reinterpret_cast<uint64_t*>(ESR_CACHE(SHIRE_OWN, bank, SC_IDX_COP_SM_CTL_USER));
    *control = (1ULL << 0) | (10ULL << 8);
  }

  for (uint32_t bank = 0U; bank < l2CacheBanks; ++bank) {
    volatile uint64_t* control = reinterpret_cast<uint64_t*>(ESR_CACHE(SHIRE_OWN, bank, SC_IDX_COP_SM_CTL_USER));
    uint64_t state = 0ULL;
    do {
      state = (*control >> 24) & 0xffULL;
    } while (state != 4ULL);
  }
}

void initializeScratchpadInputs() {
  for (uint32_t row = 0U; row < gpsdk::examples::chimera_gemv::kRows; ++row) {
    for (uint32_t col = 0U; col < gpsdk::examples::chimera_gemv::kCols; ++col) {
      storeGlobalFloat(matrixPtr(row, col), gpsdk::examples::chimera_gemv::makeMatrixValue(row, col));
    }
  }

  for (uint32_t col = 0U; col < gpsdk::examples::chimera_gemv::kCols; ++col) {
    storeGlobalFloat(vectorPtr(col), gpsdk::examples::chimera_gemv::makeVectorValue(col));
  }

  for (uint32_t row = 0U; row < gpsdk::examples::chimera_gemv::kRows; ++row) {
    storeGlobalFloat(initialOutputPtr(row), gpsdk::examples::chimera_gemv::makeInitialOutputValue(row));
  }
}

void runSimdGemv(Result* result) {
  alignas(32) float matrixLanes[kPackedLanes];
  alignas(32) float vectorLanes[kPackedLanes];

  for (uint32_t row = 0U; row < gpsdk::examples::chimera_gemv::kRows; ++row) {
    float dot = 0.0f;

    for (uint32_t col = 0U; col < gpsdk::examples::chimera_gemv::kCols; col += kPackedLanes) {
      for (uint32_t lane = 0U; lane < kPackedLanes; ++lane) {
        matrixLanes[lane] = loadGlobalFloat(matrixPtr(row, col + lane));
        vectorLanes[lane] = loadGlobalFloat(vectorPtr(col + lane));
      }

      const float aVec = packedLoad8(matrixLanes);
      const float xVec = packedLoad8(vectorLanes);
      dot += packedHorizontalSum(packedMul(aVec, xVec));
    }

    const float y0 = loadGlobalFloat(initialOutputPtr(row));
    const float outputValue = (gpsdk::examples::chimera_gemv::kAlpha * dot) + (gpsdk::examples::chimera_gemv::kBeta * y0);
    storeGlobalFloat(outputPtr(row), outputValue);
    result->outputs[row] = outputValue;
  }
}

void runTensorProbe(Result* result) {
  for (uint32_t row = 0U; row < kTensorTileRows; ++row) {
    for (uint32_t inner = 0U; inner < kTensorTileK; ++inner) {
      gTensorProbeA[row * kTensorTileK + inner] = loadGlobalFloat(matrixPtr(row, inner));
    }
  }

  for (uint32_t inner = 0U; inner < kTensorTileK; ++inner) {
    const float value = loadGlobalFloat(vectorPtr(inner));
    for (uint32_t col = 0U; col < kTensorTileCols; ++col) {
      gTensorProbeB[inner * kTensorTileCols + col] = value;
    }
  }

  for (uint32_t idx = 0U; idx < (kTensorTileRows * kTensorTileCols); ++idx) {
    gTensorProbeC[idx] = 0.0f;
  }

  tensor_load(false, false, 0U, 0U, 0U, reinterpret_cast<uint64_t>(gTensorProbeA), 0U, kTensorTileRows - 1U,
              kTensorStrideBytes, 0U);
  tensor_load(false, false, 32U, 0U, 1U, reinterpret_cast<uint64_t>(gTensorProbeB), 0U, kTensorTileK - 1U,
              kTensorStrideBytes, 1U);
  tensor_wait(TENSOR_LOAD_WAIT_0);

  tensor_fma(false, static_cast<uint64_t>((kTensorTileCols / 4U) - 1U), kTensorTileRows - 1U, kTensorTileK - 1U, 0U,
             false, false, false, true, 0U, 0U, kTensorOpcodeFp32, 1U);
  tensor_wait(TENSOR_FMA_WAIT);

  tensor_store(0U, 0U, ((kTensorTileCols * sizeof(float)) / 4U) - 1U, kTensorTileRows - 1U,
               reinterpret_cast<uint64_t>(gTensorProbeC), 0U, kTensorStrideBytes);
  tensor_wait(TENSOR_STORE_WAIT);
  drainCoalescingBuffers();

  for (uint32_t idx = 0U; idx < (kTensorTileRows * kTensorTileCols); ++idx) {
    result->tensorProbe[idx] = gTensorProbeC[idx];
  }
}

} // namespace

int entryPoint(KernelArguments* args) {
  et_assert(args != nullptr);
  et_assert(args->resultAddress != 0ULL);
  et_assert(gpsdk::device::star_scratchpad::isAvailable());
  et_assert(isScratchpadNestedStarClusterEnabled());
  et_assert(gpsdk::device::star_scratchpad::capacity() == (16ULL << 20));

  if (get_relative_thread_id() != 0) {
    return 0;
  }

  auto* result = reinterpret_cast<volatile Result*>(args->resultAddress);
  auto* mutableResult = reinterpret_cast<Result*>(args->resultAddress);

  setFullMask();
  initializeScratchpadInputs();
  runSimdGemv(mutableResult);
  runTensorProbe(mutableResult);

  result->magic = gpsdk::examples::chimera_gemv::kResultMagic;
  result->rows = gpsdk::examples::chimera_gemv::kRows;
  result->cols = gpsdk::examples::chimera_gemv::kCols;
  result->activeThreads = static_cast<uint32_t>(get_num_threads());
  result->centerShire = gpsdk::device::star_scratchpad::getCenterShireId();
  result->tensorProbeRows = kTensorTileRows;
  result->tensorProbeCols = kTensorTileCols;

  return 0;
}
