/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <cstddef>
#include <cstdint>

#include <etsoc/isa/hart.h>

#include "entryPoint.h"
#include "gemm_kernel_arguments.h"

namespace {

bool isTranspose(char trans) {
  return trans == 'T' || trans == 't' || trans == 'C' || trans == 'c';
}

float generalElement(const float* matrix, int ld, int row, int col, bool transpose) {
  return transpose ? matrix[col + static_cast<size_t>(row) * ld] : matrix[row + static_cast<size_t>(col) * ld];
}

} // namespace

int entryPoint_0(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint_0, nullptr);

int entryPoint_0(KernelArguments* args) {
  if (args->m <= 0 || args->n <= 0 || args->k < 0) {
    return 0;
  }

  if (get_relative_thread_id() != 0) {
    return 0;
  }

  const bool transA = isTranspose(args->transa);
  const bool transB = isTranspose(args->transb);

  for (int col = 0; col < args->n; ++col) {
    for (int row = 0; row < args->m; ++row) {
      float sum = 0.0f;
      for (int inner = 0; inner < args->k; ++inner) {
        sum += generalElement(args->a, args->lda, row, inner, transA) *
               generalElement(args->b, args->ldb, inner, col, transB);
      }
      float& cValue = args->c[row + static_cast<size_t>(col) * args->ldc];
      cValue = args->alpha * sum + args->beta * cValue;
    }
  }

  return 0;
}
