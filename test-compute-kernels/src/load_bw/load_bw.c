/*-------------------------------------------------------------------------
 * Copyright (C) 2018, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------
 */

#include <stdint.h>
#include <stddef.h>
#include "etsoc/isa/hart.h"
#include "etsoc/common/utils.h"
#include "common.h"

#include "etsoc/isa/cacheops.h"
#include "etsoc/isa/fcc.h"
#include "etsoc/isa/flb.h"
#include "sync_minions.h"
#include "markers.h"

#define CACHE_LINE_SIZE 8
#define FCC_FLB 2

// Load BW test.
// The test receives as input an array of addresses
// Each array is split into 1K parts, one for each minion
// Each minion accesses its own chunk and loads from these addresses

// Tensor A holds array of addresses
// Tensor B holds the size of Tensor A
// Tensor C holds the number of minions
// The number of iterations is Tensor B / Tensor C

typedef struct {
  uint64_t offset_addr;
  uint64_t array_size;
  uint64_t num_minions;
  uint64_t* out_data;
} Parameters;
int64_t main(const Parameters *const kernel_params_ptr) {
  if (kernel_params_ptr == NULL || kernel_params_ptr->offset_addr == 0 ||
      kernel_params_ptr->array_size == 0 ||
      kernel_params_ptr->num_minions == 0 ||
      kernel_params_ptr->out_data == NULL) {
    // Bad arguments
    return -1;
  }

  uint64_t hart_id = get_hart_id();
  uint64_t minion_id = hart_id >> 1;
  if (hart_id & 1) {
    return 0;
  }

  // Set marker for waveforms
  START_WAVES_MARKER;

  uint64_t array_size = kernel_params_ptr->array_size;
  uint64_t num_minions = kernel_params_ptr->num_minions;
  uint64_t num_iter = array_size / num_minions;
  volatile uint64_t *out_data = kernel_params_ptr->out_data;

  if (num_minions > 1024) {
    et_printf("Number of minions should be <= 1024");
    return -1;
  }

  if (minion_id > num_minions) {
    return 0;
  }

  uint64_t clean_addr_base = 0x8180000000;
  uint64_t sum = 0;

  // Phase 1 -- evict input tensor and laod clean lines into L3
  START_PHASE_MARKER;

  // Evict input tensor
  for (uint64_t i = 0; i < num_iter; i++) {
    uint64_t offset = (minion_id * num_iter) + i;
    uint64_t offset_addr = kernel_params_ptr->offset_addr + offset * 8;
    evict_va(0, to_Mem, offset_addr, 0, 0x40, 0);
  }
  WAIT_CACHEOPS;

  // Now load clean lines so you evict dirty lines.
  // In total bring in 4MB x 32 = 128MB
  for (uint64_t i = 0; i < 0x800; i++) {
    uint64_t *clean_addr =
        (uint64_t *)(clean_addr_base + minion_id * 0x20000 + i * 64);
    uint64_t content = *clean_addr;
    sum = sum + content;
  }

  // Prefetch tensor into L2
  for (uint64_t i = 0; i < num_iter; i++) {
    uint64_t offset = (minion_id * num_iter) + i;
    uint64_t offset_addr = kernel_params_ptr->offset_addr + offset * 8;
    prefetch_va(0, 1, offset_addr, 0, 0x40, 0);
  }

  WAIT_CACHEOPS;

  // Sync up minions across shires.
  sync_up_all_minions(minion_id, 32, FCC_FLB);

  END_PHASE_MARKER;

  // Phase 2 -- this is the actual load_bw test:
  // Fetch pointers from tensor a and load their contents
  START_PHASE_MARKER;

  // num_iter is the same as the number of addresses each minion will access
  for (uint64_t i = 0; i < num_iter; i++) {
    uint64_t offset = (minion_id * num_iter) + i;

    uint64_t *offset_addr =
        (uint64_t *)(kernel_params_ptr->offset_addr + offset * 8);
    uint64_t *final_addr = (uint64_t *)*offset_addr;
    uint64_t val = *final_addr;
    sum = sum + val;
  }

  // End marker
  END_PHASE_MARKER;

  // Put minion ID and sum into output buffer
  out_data[CACHE_LINE_SIZE * minion_id] = minion_id;
  out_data[CACHE_LINE_SIZE * minion_id + 1] = sum;
  return 0;
}
