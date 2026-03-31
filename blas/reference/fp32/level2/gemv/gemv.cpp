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
    float sum = 0.0f;
    if (!transpose) {
      for (int column = 0; column < args->n; ++column) {
        sum += args->a[outputIndex + static_cast<size_t>(column) * args->lda] *
               args->x[xStart + column * args->incx];
      }
    } else {
      for (int row = 0; row < args->m; ++row) {
        sum += args->a[static_cast<size_t>(row) + outputIndex * args->lda] *
               args->x[xStart + row * args->incx];
      }
    }

    float& yValue = args->y[yStart + outputIndex * args->incy];
    yValue = args->alpha * sum + args->beta * yValue;
  }

  return 0;
}
