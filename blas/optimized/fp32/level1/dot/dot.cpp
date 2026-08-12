/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <etsoc/common/utils.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/tensors.h>

#include "CommonCode.h"
#include "dot_kernel_arguments.h"
#include "entryPoint.h"
#include "sync.h"

#if !defined(ET_BLAS_DOT_TAIL_SCALAR) && !defined(ET_BLAS_DOT_TAIL_MASKED)
#define ET_BLAS_DOT_TAIL_MASKED 1
#endif

namespace {

constexpr size_t kVectorLanes = 8;

inline __attribute__((always_inline)) void setMaskForLaneCount(size_t activeLanes) {
  switch (activeLanes) {
  case 0:
    mask_set(0, 0x00);
    break;
  case 1:
    mask_set(0, 0x01);
    break;
  case 2:
    mask_set(0, 0x03);
    break;
  case 3:
    mask_set(0, 0x07);
    break;
  case 4:
    mask_set(0, 0x0f);
    break;
  case 5:
    mask_set(0, 0x1f);
    break;
  case 6:
    mask_set(0, 0x3f);
    break;
  case 7:
    mask_set(0, 0x7f);
    break;
  default:
    mask_set(0, 0xff);
    break;
  }
}

inline __attribute__((always_inline)) float packedHorizontalSum(float packedValue) {
  alignas(32) float lanes[kVectorLanes];
  __asm__ __volatile__("fsw.ps %[packedValue], 0(%[dst])\n"
                       :
                       : [dst] "r"(lanes), [packedValue] "f"(packedValue)
                       : "memory");

  float result = 0.0f;
  for (size_t lane = 0; lane < kVectorLanes; ++lane) {
    result += lanes[lane];
  }
  return result;
}

inline __attribute__((always_inline)) float sdotVectorCore(
  const float* x, const float* y, size_t begin, size_t end) {
  float packedAcc;
  uint32_t zeroWord = 0;
  __asm__ __volatile__("fbcx.ps %[packedAcc], %[zeroWord]\n"
                       : [packedAcc] "=&f"(packedAcc)
                       : [zeroWord] "r"(zeroWord));

  size_t i = begin;
  const size_t loopEnd = begin + ((end - begin) / kVectorLanes) * kVectorLanes;
  for (; i < loopEnd; i += kVectorLanes) {
    float xVec;
    float yVec;
    float prodVec;
    const float* xPtr = x + i;
    const float* yPtr = y + i;

    __asm__ __volatile__("flw.ps %[xVec], 0(%[xPtr])\n"
                         "flw.ps %[yVec], 0(%[yPtr])\n"
                         : [xVec] "=&f"(xVec), [yVec] "=&f"(yVec)
                         : [xPtr] "r"(xPtr), [yPtr] "r"(yPtr)
                         : "memory");

    __asm__ __volatile__("fmul.ps %[prodVec], %[xVec], %[yVec]\n"
                         "fadd.ps %[packedAcc], %[packedAcc], %[prodVec]\n"
                         : [packedAcc] "+&f"(packedAcc), [prodVec] "=&f"(prodVec)
                         : [xVec] "f"(xVec), [yVec] "f"(yVec));
  }

#if defined(ET_BLAS_DOT_TAIL_MASKED)
  const size_t remaining = end - i;
  if (remaining > 0) {
    alignas(32) float tailX[kVectorLanes] = {};
    alignas(32) float tailY[kVectorLanes] = {};
    for (size_t lane = 0; lane < remaining; ++lane) {
      tailX[lane] = x[i + lane];
      tailY[lane] = y[i + lane];
    }

    setMaskForLaneCount(remaining);

    float xVec;
    float yVec;
    float prodVec;
    __asm__ __volatile__("flw.ps %[xVec], 0(%[xPtr])\n"
                         "flw.ps %[yVec], 0(%[yPtr])\n"
                         : [xVec] "=&f"(xVec), [yVec] "=&f"(yVec)
                         : [xPtr] "r"(tailX), [yPtr] "r"(tailY)
                         : "memory");

    __asm__ __volatile__("fmul.ps %[prodVec], %[xVec], %[yVec]\n"
                         "fadd.ps %[packedAcc], %[packedAcc], %[prodVec]\n"
                         : [packedAcc] "+&f"(packedAcc), [prodVec] "=&f"(prodVec)
                         : [xVec] "f"(xVec), [yVec] "f"(yVec));

    mask_set(0, 0xff);
  }
#endif

  float localSum = packedHorizontalSum(packedAcc);

#if defined(ET_BLAS_DOT_TAIL_SCALAR)
  for (; i < end; ++i) {
    localSum += x[i] * y[i];
  }
#endif

  return localSum;
}

} // namespace

int entryPoint_0(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint_0, nullptr);

int entryPoint_0(KernelArguments* args) {
  const auto minionId = get_relative_thread_id();
  const size_t numWorkers = SOC_MINIONS_PER_SHIRE;

  if (args->numElements == 0) {
    if (minionId == 0) {
      *(args->res) = 0.0f;
    }
    return 0;
  }

  size_t elemsPerWorker = (args->numElements + numWorkers - 1) / numWorkers;
  if (elemsPerWorker % 16) {
    elemsPerWorker += 16 - (elemsPerWorker % 16);
  }

  const size_t begin = elemsPerWorker * minionId;
  const size_t end = std::min(elemsPerWorker * (minionId + 1), static_cast<size_t>(args->numElements));

  if (begin <= (args->numElements - 1)) {
    const float localSum = sdotVectorCore(args->x, args->y, begin, end);
    args->partials[begin] = localSum;
    evictCacheLine(0x1ULL, reinterpret_cast<uint8_t*>(&args->partials[begin]));
  }
  hart::barrier();

  if (minionId == 0) {
    float result = 0.0f;
    for (size_t i = 0; i < args->numElements; i += elemsPerWorker) {
      result += args->partials[i];
    }
    *(args->res) = result;
  }

  return 0;
}
