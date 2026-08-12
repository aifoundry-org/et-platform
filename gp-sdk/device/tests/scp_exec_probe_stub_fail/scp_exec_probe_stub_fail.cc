/*-------------------------------------------------------------------------
 * Copyright (c) 2026 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <etsoc/common/utils.h>

#include "entryPoint.h"

int entryPoint(void*);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint, nullptr);

int entryPoint(void*) {
  et_assert(false && "scp_exec_probe_stub_fail should never execute");
  return 0;
}
