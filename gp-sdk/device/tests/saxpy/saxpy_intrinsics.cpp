/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <etsoc/common/utils.h>
#include <etsoc/isa/hart.h>
#include <algorithm>
#include <cstdint>

#include "entryPoint.h"
#include "saxpy_kernel_arguments.h"
#include <etsoc/isa/tensors.h>

int entryPoint_0(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint_0, nullptr);

static inline __attribute__((always_inline))
void saxpy_vector(const size_t begin, const size_t end, const float alpha,
  float* x,  float* y, float* w) {

  auto i = begin;

#if defined(__clang__)
  constexpr int vlen = 8;
  using vector_t = float __attribute__((vector_size(sizeof(float) * vlen)));

  vector_t alphaVector;
  constexpr unsigned int mask = (1 << vlen) - 1;

  uint64_t alphax = __builtin_riscv_fmv_x_w(alpha);
  alphaVector = __builtin_riscv_fbcx_ps(alphax, mask);

  for (; i < end - (vlen - 1); i += vlen) {
    vector_t xValue;
    vector_t yValue;
    int *xv = reinterpret_cast<int*>(&x[i]);
    int *yv = reinterpret_cast<int*>(&y[i]);
    int *wv = reinterpret_cast<int*>(&w[i]);
    constexpr int roundingMode = 0;
    xValue = __builtin_riscv_flw_ps(0, xv, mask);
    yValue = __builtin_riscv_flw_ps(0, yv, mask);
    yValue = __builtin_riscv_fmadd_ps(alphaVector, xValue, yValue, roundingMode, mask);
    __builtin_riscv_fsw_ps(yValue, 0, wv, mask);
  }
#endif

  for (; i < end; ++i)
    w[i] = alpha * x[i] + y[i];
}

int entryPoint_0(KernelArguments* vectors) {

  auto minionId = get_relative_thread_id();
  size_t numWorkers = SOC_MINIONS_PER_SHIRE; // just 1 shire (32 minions).

  size_t elemsPerWorker = (vectors->numElements + numWorkers - 1) / numWorkers;
  if (elemsPerWorker % 16) {
    elemsPerWorker += 16 - (elemsPerWorker % 16);
  }
  size_t begin = elemsPerWorker * minionId;
  size_t end = std::min(elemsPerWorker * (minionId + 1), vectors->numElements);

  saxpy_vector(begin, end, vectors->a, vectors->x, vectors->y, vectors->y);

  return 0;
}
