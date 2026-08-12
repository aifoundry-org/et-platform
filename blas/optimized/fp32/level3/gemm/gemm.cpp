/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <algorithm>
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

  if (!(transA || transB)) {
    // Baseline optimized path:
    // - 4x4 register-resident micro-kernel over the M/N dimensions
    // - strip-mine K so the active A/B working set stays bounded
    constexpr int kBlock = 32;
    for (int colBlock = 0; colBlock < args->n; colBlock += 4) {
      const int colsInBlock = std::min(4, args->n - colBlock);
      for (int rowBlock = 0; rowBlock < args->m; rowBlock += 4) {
        const int rowsInBlock = std::min(4, args->m - rowBlock);
        float acc[4][4] = {};

        for (int kBase = 0; kBase < args->k; kBase += kBlock) {
          const int kEnd = std::min(args->k, kBase + kBlock);
          for (int inner = kBase; inner < kEnd; ++inner) {
            float aVals[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            float bVals[4] = {0.0f, 0.0f, 0.0f, 0.0f};

            for (int r = 0; r < rowsInBlock; ++r) {
              aVals[r] = args->a[(rowBlock + r) + static_cast<size_t>(inner) * args->lda];
            }
            for (int c = 0; c < colsInBlock; ++c) {
              bVals[c] = args->b[inner + static_cast<size_t>(colBlock + c) * args->ldb];
            }

            for (int r = 0; r < rowsInBlock; ++r) {
              for (int c = 0; c < colsInBlock; ++c) {
                acc[r][c] += aVals[r] * bVals[c];
              }
            }
          }
        }

        for (int c = 0; c < colsInBlock; ++c) {
          for (int r = 0; r < rowsInBlock; ++r) {
            float& cValue = args->c[(rowBlock + r) + static_cast<size_t>(colBlock + c) * args->ldc];
            cValue = args->alpha * acc[r][c] + args->beta * cValue;
          }
        }
      }
    }
    return 0;
  }

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
