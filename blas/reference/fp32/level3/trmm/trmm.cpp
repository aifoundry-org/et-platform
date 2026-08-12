/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <cstddef>
#include <cstdint>

#include <etsoc/isa/hart.h>

#include "entryPoint.h"
#include "trmm_kernel_arguments.h"

namespace {

bool isLeft(char side) {
  return side == 'L' || side == 'l';
}

bool isUpper(char uplo) {
  return uplo == 'U' || uplo == 'u';
}

bool isTranspose(char trans) {
  return trans == 'T' || trans == 't' || trans == 'C' || trans == 'c';
}

bool isUnit(char diag) {
  return diag == 'U' || diag == 'u';
}

float triangularElement(const float* a, int lda, int row, int col, bool upper, bool transpose, bool unit) {
  if (unit && row == col) {
    return 1.0f;
  }
  if (!transpose) {
    if (upper && row > col) {
      return 0.0f;
    }
    if (!upper && row < col) {
      return 0.0f;
    }
    return a[row + static_cast<size_t>(col) * lda];
  }
  if (upper && row < col) {
    return 0.0f;
  }
  if (!upper && row > col) {
    return 0.0f;
  }
  return a[col + static_cast<size_t>(row) * lda];
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
  const bool transpose = isTranspose(args->transa);
  const bool unit = isUnit(args->diag);
  const bool effectiveUpper = transpose ? !upper : upper;

  if (left) {
    const int rowStart = effectiveUpper ? 0 : (args->m - 1);
    const int rowStop = effectiveUpper ? args->m : -1;
    const int rowStep = effectiveUpper ? 1 : -1;
    for (int row = rowStart; row != rowStop; row += rowStep) {
      for (int col = 0; col < args->n; ++col) {
        float sum = 0.0f;
        for (int inner = 0; inner < args->m; ++inner) {
          sum += triangularElement(args->a, args->lda, row, inner, upper, transpose, unit) *
                 args->b[inner + static_cast<size_t>(col) * args->ldb];
        }
        args->b[row + static_cast<size_t>(col) * args->ldb] = args->alpha * sum;
      }
    }
  } else {
    const int colStart = effectiveUpper ? (args->n - 1) : 0;
    const int colStop = effectiveUpper ? -1 : args->n;
    const int colStep = effectiveUpper ? -1 : 1;
    for (int col = colStart; col != colStop; col += colStep) {
      for (int row = 0; row < args->m; ++row) {
        float sum = 0.0f;
        for (int inner = 0; inner < args->n; ++inner) {
          sum += args->b[row + static_cast<size_t>(inner) * args->ldb] *
                 triangularElement(args->a, args->lda, inner, col, upper, transpose, unit);
        }
        args->b[row + static_cast<size_t>(col) * args->ldb] = args->alpha * sum;
      }
    }
  }

  return 0;
}
