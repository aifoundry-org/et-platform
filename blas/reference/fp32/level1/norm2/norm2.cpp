/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <cstddef>

#include "CommonCode.h"
#include "entryPoint.h"
#include "norm2_kernel_arguments.h"

int entryPoint_0(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint_0, nullptr);

static inline float scalar_sqrt(float value) {
  if (value <= 0.0f) {
    return 0.0f;
  }

  float low = 0.0f;
  float high = value > 1.0f ? value : 1.0f;
  for (int iteration = 0; iteration < 32; ++iteration) {
    const float mid = 0.5f * (low + high);
    const float midSquared = mid * mid;
    if (midSquared < value) {
      low = mid;
    } else {
      high = mid;
    }
  }

  return 0.5f * (low + high);
}

int entryPoint_0(KernelArguments* args) {
  const auto minionId = get_relative_thread_id();

  if (args->numElements == 0) {
    if (minionId == 0) {
      *(args->res) = 0.0f;
    }
    return 0;
  }

  if (minionId == 0) {
    float sumSquares = 0.0f;
    for (size_t i = 0; i < args->numElements; ++i) {
      sumSquares += args->x[i] * args->x[i];
    }
    *(args->res) = scalar_sqrt(sumSquares);
  }

  return 0;
}
