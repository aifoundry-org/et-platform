/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <cstdint>

#include <etsoc/isa/hart.h>

#include "entryPoint.h"
#include "ger_kernel_arguments.h"

namespace {

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

  if (get_relative_thread_id() != 0) {
    return 0;
  }

  const int xStart = startIndex(args->m, args->incx);
  const int yStart = startIndex(args->n, args->incy);

  for (int column = 0; column < args->n; ++column) {
    const float yValue = args->y[yStart + column * args->incy];
    for (int row = 0; row < args->m; ++row) {
      args->a[row + static_cast<size_t>(column) * args->lda] +=
        args->alpha * args->x[xStart + row * args->incx] * yValue;
    }
  }

  return 0;
}
