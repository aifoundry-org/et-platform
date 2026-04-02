/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <etsoc/common/utils.h>
#include <etsoc/isa/hart.h>

#include "entryPoint.h"
#include "trsm_kernel_arguments.h"

namespace {

bool isLeftSide(char side) {
  return side == 'L' || side == 'l';
}

bool isUpper(char uplo) {
  return uplo == 'U' || uplo == 'u';
}

bool isTransposed(char trans) {
  return trans == 'T' || trans == 't' || trans == 'C' || trans == 'c';
}

bool isUnitDiag(char diag) {
  return diag == 'U' || diag == 'u';
}

} // namespace

int entryPoint_0(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint_0, nullptr);

int entryPoint_0(KernelArguments* args) {
  if (args->m <= 0 || args->n <= 0) {
    return 0;
  }

  const bool leftSide = isLeftSide(args->side);
  const bool upper = isUpper(args->uplo);
  const bool transA = isTransposed(args->transa);
  const bool unitDiag = isUnitDiag(args->diag);

  const auto minionId = get_relative_thread_id();
  const size_t numWorkers = SOC_MINIONS_PER_SHIRE;

  // Work distribution: each minion handles a subset of columns (for left) or rows (for right)
  // This is necessary because TRSM has row/column dependencies

  if (leftSide) {
    // B = alpha * A^-1 * B (solve A * X = alpha * B for X)
    // For upper triangular, solve from bottom to top
    // For lower triangular, solve from top to bottom

    size_t colsPerWorker = (static_cast<size_t>(args->n) + numWorkers - 1) / numWorkers;
    if (colsPerWorker % 16) {
      colsPerWorker += 16 - (colsPerWorker % 16);
    }

    const size_t colBegin = colsPerWorker * minionId;
    const size_t colEnd = std::min(colsPerWorker * (minionId + 1), static_cast<size_t>(args->n));

    if (colBegin >= static_cast<size_t>(args->n)) {
      return 0;
    }

    for (size_t j = colBegin; j < colEnd; ++j) {
      if (!transA) {
        if (upper) {
          // Solve A * x = b where A is upper triangular
          // Back substitution from bottom
          for (int64_t i = args->m - 1; i >= 0; --i) {
            float sum = args->alpha * args->b[static_cast<size_t>(i) + static_cast<size_t>(j) * args->ldb];
            for (int64_t k = i + 1; k < args->m; ++k) {
              sum -= args->a[static_cast<size_t>(i) + static_cast<size_t>(k) * args->lda] *
                     args->b[static_cast<size_t>(k) + static_cast<size_t>(j) * args->ldb];
            }
            if (!unitDiag) {
              sum /= args->a[static_cast<size_t>(i) + static_cast<size_t>(i) * args->lda];
            }
            args->b[static_cast<size_t>(i) + static_cast<size_t>(j) * args->ldb] = sum;
          }
        } else {
          // Solve A * x = b where A is lower triangular
          // Forward substitution from top
          for (int64_t i = 0; i < args->m; ++i) {
            float sum = args->alpha * args->b[static_cast<size_t>(i) + static_cast<size_t>(j) * args->ldb];
            for (int64_t k = 0; k < i; ++k) {
              sum -= args->a[static_cast<size_t>(i) + static_cast<size_t>(k) * args->lda] *
                     args->b[static_cast<size_t>(k) + static_cast<size_t>(j) * args->ldb];
            }
            if (!unitDiag) {
              sum /= args->a[static_cast<size_t>(i) + static_cast<size_t>(i) * args->lda];
            }
            args->b[static_cast<size_t>(i) + static_cast<size_t>(j) * args->ldb] = sum;
          }
        }
      } else {
        // Solve A^T * x = b
        if (upper) {
          // A^T is lower, so forward substitution
          for (int64_t i = 0; i < args->m; ++i) {
            float sum = args->alpha * args->b[static_cast<size_t>(i) + static_cast<size_t>(j) * args->ldb];
            for (int64_t k = 0; k < i; ++k) {
              sum -= args->a[static_cast<size_t>(k) + static_cast<size_t>(i) * args->lda] *
                     args->b[static_cast<size_t>(k) + static_cast<size_t>(j) * args->ldb];
            }
            if (!unitDiag) {
              sum /= args->a[static_cast<size_t>(i) + static_cast<size_t>(i) * args->lda];
            }
            args->b[static_cast<size_t>(i) + static_cast<size_t>(j) * args->ldb] = sum;
          }
        } else {
          // A^T is upper, so back substitution
          for (int64_t i = args->m - 1; i >= 0; --i) {
            float sum = args->alpha * args->b[static_cast<size_t>(i) + static_cast<size_t>(j) * args->ldb];
            for (int64_t k = i + 1; k < args->m; ++k) {
              sum -= args->a[static_cast<size_t>(k) + static_cast<size_t>(i) * args->lda] *
                     args->b[static_cast<size_t>(k) + static_cast<size_t>(j) * args->ldb];
            }
            if (!unitDiag) {
              sum /= args->a[static_cast<size_t>(i) + static_cast<size_t>(i) * args->lda];
            }
            args->b[static_cast<size_t>(i) + static_cast<size_t>(j) * args->ldb] = sum;
          }
        }
      }
    }
  } else {
    // B = alpha * B * A^-1 (solve X * A = alpha * B for X)
    // For upper triangular, solve from left to right
    // For lower triangular, solve from right to left

    size_t rowsPerWorker = (static_cast<size_t>(args->m) + numWorkers - 1) / numWorkers;
    if (rowsPerWorker % 16) {
      rowsPerWorker += 16 - (rowsPerWorker % 16);
    }

    const size_t rowBegin = rowsPerWorker * minionId;
    const size_t rowEnd = std::min(rowsPerWorker * (minionId + 1), static_cast<size_t>(args->m));

    if (rowBegin >= static_cast<size_t>(args->m)) {
      return 0;
    }

    for (size_t i = rowBegin; i < rowEnd; ++i) {
      if (!transA) {
        if (upper) {
          // Solve x * A = b where A is upper triangular
          // Forward substitution from left
          for (int64_t j = 0; j < args->n; ++j) {
            float sum = args->alpha * args->b[i + static_cast<size_t>(j) * args->ldb];
            for (int64_t k = 0; k < j; ++k) {
              sum -= args->b[i + static_cast<size_t>(k) * args->ldb] *
                     args->a[static_cast<size_t>(k) + static_cast<size_t>(j) * args->lda];
            }
            if (!unitDiag) {
              sum /= args->a[static_cast<size_t>(j) + static_cast<size_t>(j) * args->lda];
            }
            args->b[i + static_cast<size_t>(j) * args->ldb] = sum;
          }
        } else {
          // Solve x * A = b where A is lower triangular
          // Back substitution from right
          for (int64_t j = args->n - 1; j >= 0; --j) {
            float sum = args->alpha * args->b[i + static_cast<size_t>(j) * args->ldb];
            for (int64_t k = j + 1; k < args->n; ++k) {
              sum -= args->b[i + static_cast<size_t>(k) * args->ldb] *
                     args->a[static_cast<size_t>(k) + static_cast<size_t>(j) * args->lda];
            }
            if (!unitDiag) {
              sum /= args->a[static_cast<size_t>(j) + static_cast<size_t>(j) * args->lda];
            }
            args->b[i + static_cast<size_t>(j) * args->ldb] = sum;
          }
        }
      } else {
        // Solve x * A^T = b
        if (upper) {
          // A^T is lower
          for (int64_t j = args->n - 1; j >= 0; --j) {
            float sum = args->alpha * args->b[i + static_cast<size_t>(j) * args->ldb];
            for (int64_t k = j + 1; k < args->n; ++k) {
              sum -= args->b[i + static_cast<size_t>(k) * args->ldb] *
                     args->a[static_cast<size_t>(j) + static_cast<size_t>(k) * args->lda];
            }
            if (!unitDiag) {
              sum /= args->a[static_cast<size_t>(j) + static_cast<size_t>(j) * args->lda];
            }
            args->b[i + static_cast<size_t>(j) * args->ldb] = sum;
          }
        } else {
          // A^T is upper
          for (int64_t j = 0; j < args->n; ++j) {
            float sum = args->alpha * args->b[i + static_cast<size_t>(j) * args->ldb];
            for (int64_t k = 0; k < j; ++k) {
              sum -= args->b[i + static_cast<size_t>(k) * args->ldb] *
                     args->a[static_cast<size_t>(j) + static_cast<size_t>(k) * args->lda];
            }
            if (!unitDiag) {
              sum /= args->a[static_cast<size_t>(j) + static_cast<size_t>(j) * args->lda];
            }
            args->b[i + static_cast<size_t>(j) * args->ldb] = sum;
          }
        }
      }
    }
  }

  return 0;
}
