/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <etsoc/common/utils.h>
#include <etsoc/isa/tensors.h>

#include "entryPoint.h"
#include "gemv_kernel_arguments.h"

namespace {

constexpr size_t kVectorLanes = 8;

bool isTranspose(char trans) {
  return trans == 'T' || trans == 't' || trans == 'C' || trans == 'c';
}

int startIndex(int length, int inc) {
  return inc >= 0 ? 0 : (length - 1) * (-inc);
}

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

inline __attribute__((always_inline)) float broadcastPackedFloat(float value) {
  uint32_t bits = 0;
  __builtin_memcpy(&bits, &value, sizeof(bits));

  float packedValue;
  __asm__ __volatile__("fbcx.ps %[packedValue], %[bits]\n"
                       : [packedValue] "=&f"(packedValue)
                       : [bits] "r"(bits));
  return packedValue;
}

} // namespace

int entryPoint_0(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint_0, nullptr);

int entryPoint_0(KernelArguments* args) {
  if (args->m <= 0 || args->n <= 0) {
    return 0;
  }

  const bool transpose = isTranspose(args->trans);
  const int outputCount = transpose ? args->n : args->m;
  const int reductionCount = transpose ? args->m : args->n;
  const int xStart = startIndex(reductionCount, args->incx);
  const int yStart = startIndex(outputCount, args->incy);

  const auto minionId = get_relative_thread_id();
  const size_t numWorkers = SOC_MINIONS_PER_SHIRE;
  size_t elemsPerWorker = (static_cast<size_t>(outputCount) + numWorkers - 1) / numWorkers;
  if (elemsPerWorker % 16) {
    elemsPerWorker += 16 - (elemsPerWorker % 16);
  }

  const size_t begin = elemsPerWorker * minionId;
  const size_t end = std::min(elemsPerWorker * (minionId + 1), static_cast<size_t>(outputCount));
  if (begin > static_cast<size_t>(outputCount - 1)) {
    return 0;
  }

  const bool usePackedVectorPath = !transpose && args->incx == 1 && args->incy == 1;

  if (usePackedVectorPath) {
    const float packedAlpha = broadcastPackedFloat(args->alpha);
    const float packedBeta = broadcastPackedFloat(args->beta);

    for (size_t rowBase = begin; rowBase < end; rowBase += kVectorLanes) {
      const size_t rowsInBlock = std::min(kVectorLanes, end - rowBase);
      setMaskForLaneCount(rowsInBlock);

      float accVec = broadcastPackedFloat(0.0f);
      for (int column = 0; column < args->n; ++column) {
        const float* aPtr = args->a + rowBase + static_cast<size_t>(column) * args->lda;
        const float* xPtr = args->x + xStart + column;

        float aVec;
        float xVec;
        float prodVec;

        uint32_t xBits = 0;
        __builtin_memcpy(&xBits, xPtr, sizeof(xBits));

        __asm__ __volatile__("flw.ps %[aVec], 0(%[aPtr])\n"
                             "fbcx.ps %[xVec], %[xBits]\n"
                             "fmul.ps %[prodVec], %[aVec], %[xVec]\n"
                             "fadd.ps %[accVec], %[accVec], %[prodVec]\n"
                             : [accVec] "+&f"(accVec), [aVec] "=&f"(aVec), [xVec] "=&f"(xVec),
                               [prodVec] "=&f"(prodVec)
                             : [aPtr] "r"(aPtr), [xBits] "r"(xBits)
                             : "memory");
      }

      float yVec;
      float scaledAccVec;
      float scaledYVec;
      float outVec;
      float* yPtr = args->y + yStart + static_cast<int>(rowBase);

      __asm__ __volatile__("flw.ps %[yVec], 0(%[yPtr])\n"
                           "fmul.ps %[scaledAccVec], %[accVec], %[packedAlpha]\n"
                           "fmul.ps %[scaledYVec], %[yVec], %[packedBeta]\n"
                           "fadd.ps %[outVec], %[scaledAccVec], %[scaledYVec]\n"
                           "fsw.ps %[outVec], 0(%[yPtr])\n"
                           : [yVec] "=&f"(yVec), [scaledAccVec] "=&f"(scaledAccVec),
                             [scaledYVec] "=&f"(scaledYVec), [outVec] "=&f"(outVec)
                           : [accVec] "f"(accVec), [packedAlpha] "f"(packedAlpha),
                             [packedBeta] "f"(packedBeta), [yPtr] "r"(yPtr)
                           : "memory");
    }

    mask_set(0, 0xff);
    return 0;
  }

  for (size_t outputIndex = begin; outputIndex < end; ++outputIndex) {
    float sum0 = 0.0f;
    float sum1 = 0.0f;
    float sum2 = 0.0f;
    float sum3 = 0.0f;
    int inner = 0;

    if (!transpose) {
      const float* matrixRow = args->a + outputIndex;
      const int loopEnd = (args->n / 4) * 4;
      for (; inner < loopEnd; inner += 4) {
        sum0 += matrixRow[static_cast<size_t>(inner + 0) * args->lda] *
                args->x[xStart + (inner + 0) * args->incx];
        sum1 += matrixRow[static_cast<size_t>(inner + 1) * args->lda] *
                args->x[xStart + (inner + 1) * args->incx];
        sum2 += matrixRow[static_cast<size_t>(inner + 2) * args->lda] *
                args->x[xStart + (inner + 2) * args->incx];
        sum3 += matrixRow[static_cast<size_t>(inner + 3) * args->lda] *
                args->x[xStart + (inner + 3) * args->incx];
      }
      float sum = sum0 + sum1 + sum2 + sum3;
      for (; inner < args->n; ++inner) {
        sum += matrixRow[static_cast<size_t>(inner) * args->lda] * args->x[xStart + inner * args->incx];
      }
      float& yValue = args->y[yStart + static_cast<int>(outputIndex) * args->incy];
      yValue = args->alpha * sum + args->beta * yValue;
    } else {
      const int loopEnd = (args->m / 4) * 4;
      for (; inner < loopEnd; inner += 4) {
        sum0 += args->a[static_cast<size_t>(inner + 0) + outputIndex * args->lda] *
                args->x[xStart + (inner + 0) * args->incx];
        sum1 += args->a[static_cast<size_t>(inner + 1) + outputIndex * args->lda] *
                args->x[xStart + (inner + 1) * args->incx];
        sum2 += args->a[static_cast<size_t>(inner + 2) + outputIndex * args->lda] *
                args->x[xStart + (inner + 2) * args->incx];
        sum3 += args->a[static_cast<size_t>(inner + 3) + outputIndex * args->lda] *
                args->x[xStart + (inner + 3) * args->incx];
      }
      float sum = sum0 + sum1 + sum2 + sum3;
      for (; inner < args->m; ++inner) {
        sum += args->a[static_cast<size_t>(inner) + outputIndex * args->lda] *
               args->x[xStart + inner * args->incx];
      }
      float& yValue = args->y[yStart + static_cast<int>(outputIndex) * args->incy];
      yValue = args->alpha * sum + args->beta * yValue;
    }
  }

  return 0;
}
