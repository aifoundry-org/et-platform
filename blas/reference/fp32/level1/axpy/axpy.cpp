/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <algorithm>
#include <cstddef>

#include <etsoc/common/utils.h>
#include <etsoc/isa/hart.h>

#include "axpy_kernel_arguments.h"
#include "entryPoint.h"

int entryPoint_0(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint_0, nullptr);

int entryPoint_0(KernelArguments* args) {
  const auto minionId = get_relative_thread_id();
  const size_t numWorkers = SOC_MINIONS_PER_SHIRE;

  size_t elemsPerWorker = (args->numElements + numWorkers - 1) / numWorkers;
  if (elemsPerWorker == 0) {
    return 0;
  }

  const size_t begin = elemsPerWorker * minionId;
  const size_t end = std::min(begin + elemsPerWorker, static_cast<size_t>(args->numElements));
  if (begin >= args->numElements) {
    return 0;
  }

  for (size_t i = begin; i < end; ++i) {
    args->y[i] = args->alpha * args->x[i] + args->y[i];
  }

  return 0;
}
