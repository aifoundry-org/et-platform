/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <cstddef>
#include <cstdint>

#include <etsoc/isa/hart.h>

#include "entryPoint.h"
#include "symm_kernel_arguments.h"

namespace {

bool isLeft(char side) {
  return side == 'L' || side == 'l';
}

bool isUpper(char uplo) {
  return uplo == 'U' || uplo == 'u';
}

float symmetricElement(const float* matrix, int ld, int row, int col, bool upper) {
  if (upper) {
    return row <= col ? matrix[row + static_cast<size_t>(col) * ld]
                      : matrix[col + static_cast<size_t>(row) * ld];
  }
  return row >= col ? matrix[row + static_cast<size_t>(col) * ld]
                    : matrix[col + static_cast<size_t>(row) * ld];
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

  const bool left = isLeft(args->side);
  const bool upper = isUpper(args->uplo);

  for (int col = 0; col < args->n; ++col) {
    for (int row = 0; row < args->m; ++row) {
      float sum = 0.0f;
      if (left) {
        for (int inner = 0; inner < args->m; ++inner) {
          sum += symmetricElement(args->a, args->lda, row, inner, upper) *
                 args->b[inner + static_cast<size_t>(col) * args->ldb];
        }
      } else {
        for (int inner = 0; inner < args->n; ++inner) {
          sum += args->b[row + static_cast<size_t>(inner) * args->ldb] *
                 symmetricElement(args->a, args->lda, inner, col, upper);
        }
      }
      float& cValue = args->c[row + static_cast<size_t>(col) * args->ldc];
      cValue = args->alpha * sum + args->beta * cValue;
    }
  }

  return 0;
}
