/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <cstddef>
#include <cstdint>

#include <etsoc/isa/hart.h>

#include "entryPoint.h"
#include "syr_kernel_arguments.h"

namespace {

bool isUpper(char uplo) {
  return uplo == 'U' || uplo == 'u';
}

int startIndex(int length, int inc) {
  return inc >= 0 ? 0 : (length - 1) * (-inc);
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

  for (int col = 0; col < args->n; ++col) {
    const float xCol = args->x[xStart + col * args->incx];
    const int rowBegin = upper ? 0 : col;
    const int rowEnd = upper ? (col + 1) : args->n;
    for (int row = rowBegin; row < rowEnd; ++row) {
      args->a[row + static_cast<size_t>(col) * args->lda] +=
        args->alpha * args->x[xStart + row * args->incx] * xCol;
    }
  }

  return 0;
}
