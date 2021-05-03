
/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include <stdint.h>
typedef struct {
  uint64_t num_iters;
} Parameters;

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

int main(const Parameters* const params) {
  dummyLoop(params->num_iters);
  return 0;
}
