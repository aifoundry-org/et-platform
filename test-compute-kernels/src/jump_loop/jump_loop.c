
/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/
#include <stdint.h>
typedef struct {
  uint64_t num_iters;
} Parameters;

int entry_point(const Parameters*);

// Function that returns shire id
inline __attribute__((always_inline)) void dummyLoop(uint64_t num_iters) {
  __asm__ __volatile__(
      ".START_LOOP:\n\t"
      "bge %1, %0, .END_LOOP\n\t"
      "addi %1, %1, 1\n\t"
      "j .START_LOOP\n"
      ".END_LOOP:"
      : /* no outputs */
      : "r"(num_iters), "r"(0));
}

int entry_point(const Parameters* const params) {
  dummyLoop(params->num_iters);
  return 0;
}
