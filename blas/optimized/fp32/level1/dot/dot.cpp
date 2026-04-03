/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <algorithm>
#include <cstddef>

#include <etsoc/common/utils.h>
#include <etsoc/isa/hart.h>

#include "CommonCode.h"
#include "dot_kernel_arguments.h"
#include "entryPoint.h"
#include "sync.h"

int entryPoint_0(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint_0, nullptr);

int entryPoint_0(KernelArguments* args) {
  const auto minionId = get_relative_thread_id();
  const size_t numWorkers = SOC_MINIONS_PER_SHIRE;

  if (args->numElements == 0) {
    if (minionId == 0) {
      *(args->res) = 0.0f;
    }
    return 0;
  }

  size_t elemsPerWorker = (args->numElements + numWorkers - 1) / numWorkers;
  if (elemsPerWorker % 16) {
    elemsPerWorker += 16 - (elemsPerWorker % 16);
  }

  const size_t begin = elemsPerWorker * minionId;
  const size_t end = std::min(elemsPerWorker * (minionId + 1), static_cast<size_t>(args->numElements));

  if (begin <= (args->numElements - 1)) {
    // First optimized step: modest scalar unrolling with independent
    // accumulators. This preserves the reference kernel contract and reduction
    // shape while exposing more instruction-level parallelism.
    size_t i = begin;
    float sum0 = 0.0f;
    float sum1 = 0.0f;
    float sum2 = 0.0f;
    float sum3 = 0.0f;
    const size_t loopEnd = begin + ((end - begin) / 4) * 4;
    for (; i < loopEnd; i += 4) {
      sum0 += args->x[i + 0] * args->y[i + 0];
      sum1 += args->x[i + 1] * args->y[i + 1];
      sum2 += args->x[i + 2] * args->y[i + 2];
      sum3 += args->x[i + 3] * args->y[i + 3];
    }

    float localSum = sum0 + sum1 + sum2 + sum3;
    for (; i < end; ++i) {
      localSum += args->x[i] * args->y[i];
    }
    args->partials[begin] = localSum;
    evictCacheLine(0x1ULL, reinterpret_cast<uint8_t*>(&args->partials[begin]));
  }
  hart::barrier();

  if (minionId == 0) {
    float result = 0.0f;
    for (size_t i = 0; i < args->numElements; i += elemsPerWorker) {
      result += args->partials[i];
    }
    *(args->res) = result;
  }

  return 0;
}
