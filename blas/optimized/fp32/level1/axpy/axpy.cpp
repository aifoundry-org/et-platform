/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <algorithm>
#include <cstddef>

#include <etsoc/common/utils.h>

#include "axpy_kernel_arguments.h"
#include "entryPoint.h"

int entryPoint_0(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint_0, nullptr);

int entryPoint_0(KernelArguments* args) {
  const auto minionId = get_relative_thread_id();
  const size_t numWorkers = SOC_MINIONS_PER_SHIRE;

  if (args->numElements == 0) {
    return 0;
  }

  size_t elemsPerWorker = (args->numElements + numWorkers - 1) / numWorkers;
  if (elemsPerWorker % 16) {
    elemsPerWorker += 16 - (elemsPerWorker % 16);
  }

  const size_t begin = elemsPerWorker * minionId;
  const size_t end = std::min(elemsPerWorker * (minionId + 1), static_cast<size_t>(args->numElements));
  if (begin > (args->numElements - 1)) {
    return 0;
  }

  size_t i = begin;
  const size_t loopEnd = begin + ((end - begin) / 4) * 4;
  for (; i < loopEnd; i += 4) {
    args->y[i + 0] = args->alpha * args->x[i + 0] + args->y[i + 0];
    args->y[i + 1] = args->alpha * args->x[i + 1] + args->y[i + 1];
    args->y[i + 2] = args->alpha * args->x[i + 2] + args->y[i + 2];
    args->y[i + 3] = args->alpha * args->x[i + 3] + args->y[i + 3];
  }

  for (; i < end; ++i) {
    args->y[i] = args->alpha * args->x[i] + args->y[i];
  }

  return 0;
}
