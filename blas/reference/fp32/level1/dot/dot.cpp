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
#include "entryPoint.h"
#include "dot_kernel_arguments.h"
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
    float localSum = 0.0f;
    for (size_t i = begin; i < end; ++i) {
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
