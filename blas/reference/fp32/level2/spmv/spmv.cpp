/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <cstddef>
#include <cstdint>

#include <etsoc/isa/hart.h>

#include "entryPoint.h"
#include "spmv_kernel_arguments.h"

namespace {

bool isUpper(char uplo) {
  return uplo == 'U' || uplo == 'u';
}

int startIndex(int length, int inc) {
  return inc >= 0 ? 0 : (length - 1) * (-inc);
}

size_t upperPackedIndex(int row, int col) {
  return static_cast<size_t>(col) * (col + 1) / 2 + row;
}

size_t lowerPackedOffset(int n, int col) {
  return static_cast<size_t>(col) * n - static_cast<size_t>(col) * (col - 1) / 2;
}

float symmetricPackedElement(const float* ap, int n, int row, int col, bool upper) {
  if (upper) {
    return row <= col ? ap[upperPackedIndex(row, col)] : ap[upperPackedIndex(col, row)];
  }
  return row >= col ? ap[lowerPackedOffset(n, col) + (row - col)]
                    : ap[lowerPackedOffset(n, row) + (col - row)];
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
      sum += symmetricPackedElement(args->ap, args->n, row, col, upper) * args->x[xStart + col * args->incx];
    }
    float& yValue = args->y[yStart + row * args->incy];
    yValue = args->alpha * sum + args->beta * yValue;
  }

  return 0;
}
