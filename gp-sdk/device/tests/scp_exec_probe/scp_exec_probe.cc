/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <cstdint>

#include <etsoc/common/utils.h>

#include "CommonCode.h"
#include "entryPoint.h"
#include "gpsdk_scp_exec_probe.h"

using gpsdk::examples::scp_exec_probe::KernelArguments;
using gpsdk::examples::scp_exec_probe::Result;

int entryPoint(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint, nullptr);

int entryPoint(KernelArguments* args) {
  et_assert(args != nullptr);
  et_assert(args->resultAddress != 0ULL);

  if (get_relative_thread_id() != 0) {
    return 0;
  }

  auto* result = reinterpret_cast<volatile Result*>(args->resultAddress);
  result->started = 1U;
  result->completed = 0U;
  result->centerShire = getEffectiveCenterShire();
  result->threadId = static_cast<uint32_t>(get_relative_thread_id());
  result->expectedEntryAddress = args->expectedEntryAddress;
  result->observedEntryAddress = reinterpret_cast<uint64_t>(&entryPoint);

  if (result->observedEntryAddress != args->expectedEntryAddress) {
    return 0;
  }

  result->magic = gpsdk::examples::scp_exec_probe::kResultMagic;
  result->completed = 1U;
  return 0;
}
