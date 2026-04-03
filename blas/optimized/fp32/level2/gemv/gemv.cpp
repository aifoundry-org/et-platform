/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <etsoc/common/utils.h>

#include "entryPoint.h"
#include "gemv_kernel_arguments.h"

namespace {

bool isTranspose(char trans) {
  return trans == 'T' || trans == 't' || trans == 'C' || trans == 'c';
}

int startIndex(int length, int inc) {
  return inc >= 0 ? 0 : (length - 1) * (-inc);
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
