/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <etsoc/common/utils.h>
#include <etsoc/isa/hart.h>

#include "CommonCode.h"
#include "entryPoint.h"
#include "etsoc/isa/atomic.h"


class KernelArguments;
int entryPoint(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint, entryPoint);

// Constructors
static void setupGlobal2(void) __attribute__((constructor(102)));
static void setupGlobal1(void) __attribute__((constructor(101)));

// globals written by the constructors
static unsigned int global_value1_with_constructor = 1;
static unsigned int global_value2_with_constructor = 2;

static void setupGlobal2(void) {
  // assert we follow the right order
  et_assert(atomic_load_global_32(&global_value1_with_constructor) == (0x1 | 0x100000));
  et_assert(atomic_load_global_32(&global_value2_with_constructor) == (0x2 | 0x100000));

  atomic_or_global_32(&global_value1_with_constructor, 0x200);
  atomic_or_global_32(&global_value2_with_constructor, 0x200);
}

static void setupGlobal1(void) {
  atomic_or_global_32(&global_value1_with_constructor, 0x100000);
  atomic_or_global_32(&global_value2_with_constructor, 0x100000);
}

int entryPoint([[maybe_unused]] KernelArguments* args) {
  // assert all the contructors have been called.
  et_assert(global_value1_with_constructor == (1 | 0x200 | 0x100000));
  et_assert(global_value2_with_constructor == (2 | 0x200 | 0x100000));
  return 0;
}
