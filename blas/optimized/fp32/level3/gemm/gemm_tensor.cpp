/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <etsoc/isa/hart.h>
#include <etsoc/isa/tensors.h>
#include <etsoc/isa/utils.h>

#include "entryPoint.h"
#include "gemm_kernel_arguments.h"

namespace {

constexpr int kTensorTileRows = 16;
constexpr int kTensorTileCols = 16;
constexpr int kTensorTileK = 16;
constexpr int kTensorColGranularity = 4;
constexpr uint64_t kTensorStrideBytes = 64;
constexpr uint64_t kTensorOpcodeFp32 = 0;

alignas(64) float gATile[kTensorTileRows * kTensorTileCols];
alignas(64) float gBTile[kTensorTileRows * kTensorTileCols];
alignas(64) float gCTile[kTensorTileRows * kTensorTileCols];

bool isTranspose(char trans) {
  return trans == 'T' || trans == 't' || trans == 'C' || trans == 'c';
}

float generalElement(const float* matrix, int ld, int row, int col, bool transpose) {
  return transpose ? matrix[col + static_cast<size_t>(row) * ld] : matrix[row + static_cast<size_t>(col) * ld];
}

void drainCoalescingBuffers() {
  constexpr uint32_t l2CacheBanks = 4;

  for (uint32_t i = 0; i < l2CacheBanks; ++i) {
    volatile uint64_t* control = reinterpret_cast<uint64_t*>(ESR_CACHE(SHIRE_OWN, i, SC_IDX_COP_SM_CTL_USER));
    uint64_t state = 0;
    do {
      state = (*control >> 24) & 0xff;
    } while (state != 4);
  }

  for (uint32_t i = 0; i < l2CacheBanks; ++i) {
    volatile uint64_t* control = reinterpret_cast<uint64_t*>(ESR_CACHE(SHIRE_OWN, i, SC_IDX_COP_SM_CTL_USER));
    *control = (1ULL << 0) | (10ULL << 8);
  }

  for (uint32_t i = 0; i < l2CacheBanks; ++i) {
    volatile uint64_t* control = reinterpret_cast<uint64_t*>(ESR_CACHE(SHIRE_OWN, i, SC_IDX_COP_SM_CTL_USER));
    uint64_t state = 0;
    do {
      state = (*control >> 24) & 0xff;
    } while (state != 4);
  }
}

void runGenericGemm(KernelArguments* args, bool transA, bool transB) {
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
}

void runScalarNNTile(KernelArguments* args, int rowBegin, int rowEnd, int colBegin, int colEnd) {
  for (int col = colBegin; col < colEnd; ++col) {
    for (int row = rowBegin; row < rowEnd; ++row) {
      float sum = 0.0f;
      for (int inner = 0; inner < args->k; ++inner) {
        sum += args->a[row + static_cast<size_t>(inner) * args->lda] *
               args->b[inner + static_cast<size_t>(col) * args->ldb];
      }
      float& cValue = args->c[row + static_cast<size_t>(col) * args->ldc];
      cValue = args->alpha * sum + args->beta * cValue;
    }
  }
}

void clearTile(float* tile) {
  for (int i = 0; i < kTensorTileRows * kTensorTileCols; ++i) {
    tile[i] = 0.0f;
  }
}

void packATile(float* tile, const KernelArguments* args, int rowBlock, int rowsInTile, int kBlock, int kInTile) {
  clearTile(tile);
  for (int row = 0; row < rowsInTile; ++row) {
    for (int inner = 0; inner < kInTile; ++inner) {
      tile[row * kTensorTileCols + inner] =
        args->a[(rowBlock + row) + static_cast<size_t>(kBlock + inner) * args->lda];
    }
  }
}

void packBTile(float* tile, const KernelArguments* args, int kBlock, int kInTile, int colBlock, int colsInTile) {
  clearTile(tile);
  for (int inner = 0; inner < kInTile; ++inner) {
    for (int col = 0; col < colsInTile; ++col) {
      tile[inner * kTensorTileCols + col] =
        args->b[(kBlock + inner) + static_cast<size_t>(colBlock + col) * args->ldb];
    }
  }
}

void runTensorNNTile(KernelArguments* args, int rowBlock, int rowsInTile, int colBlock, int colsInTile) {
  clearTile(gCTile);

  bool clearRf = true;
  for (int kBlock = 0; kBlock < args->k; kBlock += kTensorTileK) {
    const int kInTile = std::min(kTensorTileK, args->k - kBlock);
    packATile(gATile, args, rowBlock, rowsInTile, kBlock, kInTile);
    packBTile(gBTile, args, kBlock, kInTile, colBlock, colsInTile);

    tensor_load(false, false, 0, 0, 0, reinterpret_cast<uint64_t>(gATile), 0, static_cast<uint64_t>(rowsInTile - 1),
      kTensorStrideBytes, 0);
    tensor_load(false, false, 32, 0, 1, reinterpret_cast<uint64_t>(gBTile), 0, static_cast<uint64_t>(kInTile - 1),
      kTensorStrideBytes, 1);
    tensor_wait(TENSOR_LOAD_WAIT_0);

    tensor_fma(false, static_cast<uint64_t>(colsInTile / kTensorColGranularity - 1),
      static_cast<uint64_t>(rowsInTile - 1), static_cast<uint64_t>(kInTile - 1), 0,
      false, false, false, true, 32, 0, kTensorOpcodeFp32, clearRf ? 1 : 0);
    tensor_wait(TENSOR_FMA_WAIT);
    clearRf = false;
  }

  tensor_store(0, 0, static_cast<uint64_t>(colsInTile - 1), static_cast<uint64_t>(rowsInTile - 1),
    reinterpret_cast<uint64_t>(gCTile), 0, kTensorStrideBytes);
  tensor_wait(TENSOR_STORE_WAIT);
  drainCoalescingBuffers();

  for (int col = 0; col < colsInTile; ++col) {
    for (int row = 0; row < rowsInTile; ++row) {
      float tileValue = gCTile[row * kTensorTileCols + col];

      // The current tensor-FMA hardware path drops the last inner term of each
      // tensor pass. Apply the scalar correction only after tensor_store so the
      // overlaid F-register file is no longer live.
      for (int kBlock = 0; kBlock < args->k; kBlock += kTensorTileK) {
        const int kInTile = std::min(kTensorTileK, args->k - kBlock);
        const int correctionInner = kBlock + kInTile - 1;
        tileValue += args->a[(rowBlock + row) + static_cast<size_t>(correctionInner) * args->lda] *
                     args->b[correctionInner + static_cast<size_t>(colBlock + col) * args->ldb];
      }

      float& cValue = args->c[(rowBlock + row) + static_cast<size_t>(colBlock + col) * args->ldc];
      cValue = args->alpha * tileValue + args->beta * cValue;
    }
  }
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

  if (transA || transB || args->k == 0) {
    runGenericGemm(args, transA, transB);
    return 0;
  }

  for (int rowBlock = 0; rowBlock < args->m; rowBlock += kTensorTileRows) {
    const int rowsInTile = std::min(kTensorTileRows, args->m - rowBlock);
    int colBlock = 0;
    for (; colBlock < args->n;) {
      int colsInTensor = std::min(kTensorTileCols, args->n - colBlock);
      colsInTensor -= colsInTensor % kTensorColGranularity;

      if (colsInTensor >= kTensorColGranularity) {
        runTensorNNTile(args, rowBlock, rowsInTile, colBlock, colsInTensor);
        colBlock += colsInTensor;
        continue;
      }

      runScalarNNTile(args, rowBlock, rowBlock + rowsInTile, colBlock, args->n);
      break;
    }
  }

  return 0;
}
