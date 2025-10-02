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

typedef struct {
  uint64_t base_addr;
  uint64_t array_size;
  uint64_t num_minions;
  uint64_t num_columns;
  uint64_t* out_data;
} Parameters;

int64_t entry_point(const Parameters*);

// Prefetch test
// Tries to reach max BW in the memshire
int64_t entry_point(const Parameters *const kernel_params_ptr) {
  if (kernel_params_ptr == NULL || kernel_params_ptr->base_addr == 0 ||
      kernel_params_ptr->array_size == 0 ||
      kernel_params_ptr->num_minions == 0 ||
      kernel_params_ptr->num_columns == 0 ||
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

  uint64_t base_addr = kernel_params_ptr->base_addr;
  uint64_t array_size = kernel_params_ptr->array_size;
  uint64_t num_minions = kernel_params_ptr->num_minions;
  uint64_t num_columns = kernel_params_ptr->num_columns;
  uint64_t num_iter =
      array_size / (num_columns * CACHE_LINE_SIZE * 8 * num_minions);
  volatile uint64_t *out_data = kernel_params_ptr->out_data;

  if (num_minions > 1024) {
    et_printf("Number of minions should be <= 1024");
    return -1;
  }

  // Sync up minions across shires.
  sync_up_all_minions(minion_id, 32, FCC_FLB);

  if ((minion_id % (1024 / num_minions)) != 0) { // was % 4
    return 0;
  }

  START_PHASE_MARKER;

  // Prefetch area of memory specified by tensr_a argument.
  // bank_ms_mc_offset: There are 128 bank/ms/mc groups. Each with 32 lines /
  // row
  //                    Based on minion id minions are mapped to a bank/ms/mc.
  //                    For example minions 0-7 (if participating all) go to
  // bank/mc/ms = 0
  // start_row_offset: For each iteration, what is the base address for all
  // minions.
  // minion_row_offset: Within the row, on which column does the minion start
  // fetching
  //                    1024 minions: 0x8000 x minion_id
  //                    512 minions: 0x10000 x (minion_id / 2)
  //                    256 minions: 0x20000 x (minion_id / 4)
  //                    128 minions: 0x40000 x (minion_id / 8)
  // num_iter: Equal to array_size / total amount fetched per iteration by all
  // minions
  for (uint64_t i = 0; i < num_iter; i++) {
    uint64_t start_row_offset = 0x20000 * (num_minions / 8) *
                                i; // divide num_minions by 8 or something ?
    uint64_t minion_row_offset =
        (array_size / num_minions) * (minion_id / (1024 / num_minions));
    uint64_t bank_mc_ms_offset = (minion_id / 8) * 0x40;
    uint64_t final_addr =
        base_addr + start_row_offset + minion_row_offset + bank_mc_ms_offset;
    prefetch_va(0, to_L1, final_addr, num_columns - 1, 0x2000, 0);
  }

  WAIT_CACHEOPS;

  END_PHASE_MARKER;

  // Put minion ID and sum into output buffer
  out_data[CACHE_LINE_SIZE * minion_id] = minion_id;
  return 0;
}
