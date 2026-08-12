/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <cstddef>
#include <cstdint>

#include <etsoc/isa/hart.h>

#include "entryPoint.h"
#include "sbmv_kernel_arguments.h"

namespace {

bool isUpper(char uplo) {
  return uplo == 'U' || uplo == 'u';
}

int startIndex(int length, int inc) {
  return inc >= 0 ? 0 : (length - 1) * (-inc);
}

float symmetricBandElement(const float* a, int k, int lda, int row, int col, bool upper) {
  if (upper) {
    if (row <= col) {
      if (col - row > k) {
        return 0.0f;
      }
      return a[k + row - col + static_cast<size_t>(col) * lda];
    }
    if (row - col > k) {
      return 0.0f;
    }
    return a[k + col - row + static_cast<size_t>(row) * lda];
  }

  if (row >= col) {
    if (row - col > k) {
      return 0.0f;
    }
    return a[row - col + static_cast<size_t>(col) * lda];
  }
  if (col - row > k) {
    return 0.0f;
  }
  return a[col - row + static_cast<size_t>(row) * lda];
}

} // namespace

int entryPoint_0(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint_0, nullptr);

int entryPoint_0(KernelArguments* args) {
  if (args->n <= 0) {
    return 0;
  }

  if (get_relative_thread_id() != 0) {
    return 0;
  }

  const bool upper = isUpper(args->uplo);
  const int xStart = startIndex(args->n, args->incx);
  const int yStart = startIndex(args->n, args->incy);

  for (int row = 0; row < args->n; ++row) {
    float sum = 0.0f;
    for (int col = 0; col < args->n; ++col) {
      sum += symmetricBandElement(args->a, args->k, args->lda, row, col, upper) *
             args->x[xStart + col * args->incx];
    }
    float& yValue = args->y[yStart + row * args->incy];
    yValue = args->alpha * sum + args->beta * yValue;
  }

  return 0;
}
