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
#include <stdbool.h>
#include "etsoc/isa/hart.h"
#include "etsoc/common/utils.h"
#include "common.h"

#include "etsoc/isa/cacheops.h"
#include "markers.h"

#define CACHE_LINE_SIZE 8
#define FCC_FLB 2

// Loat Lat Test
// Sends a sequence of addresses from one minion only touching all MS/MC/banks
// Can serve as a latency test for the memory controller + DRAM but also for NOC
// and other parts of the system.

// Tensor A holds start address
// Tensor B holds the number of iterations (addresses) - 4K needed to cover all
// possible addresses in one row
// in all MS/MC/banks
// Tensor C holds the minion id that will send the loads
// Tensor D holds a flag that shows whether we do open or closed page accesses.
// For open page we need to traverse the columns in a single row first.
// For closed page we go through all MS/MC/banks first, and the open pages close
// based on a timer.
// Tensor E holds the output which should be the minion id that sent the loads

typedef struct {
  uint64_t base_addr;
  uint64_t num_iter;
  uint64_t active_minion_id;
  uint64_t open_page;
  uint64_t* out_data;
} Parameters;

int64_t entry_point(const Parameters*);

int64_t entry_point(const Parameters *const kernel_params_ptr) {
  if (kernel_params_ptr == NULL || kernel_params_ptr->base_addr == 0 ||
      kernel_params_ptr->num_iter == 0 || kernel_params_ptr->out_data == NULL) {
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
  uint64_t num_iter = kernel_params_ptr->num_iter;
  uint64_t active_minion_id = kernel_params_ptr->active_minion_id;
  bool open_page = ((uint64_t)kernel_params_ptr->open_page) == 1;
  volatile uint64_t *out_data = kernel_params_ptr->out_data;

  if (active_minion_id > 1023) {
    et_printf("Active minion id should be  < 1024");
    return -1;
  }

  if (active_minion_id != minion_id) {
    return 0;
  }

  START_PHASE_MARKER;

  uint64_t sum = 0;
  // Closed page accesses.
  if (!open_page) {
    for (uint64_t i = 0; i < num_iter; i++) {
      uint64_t *addr = (uint64_t *)(base_addr + i * 0x40);
      uint64_t val = *addr;
      sum = sum + val;
    }
  }
  // Open page accesses. Column offset is 0x2000. Consecutive request go to
  // consecutive columns, same row
  // Then we switch to the same row of another MS/MC/bank.
  else {
    for (uint64_t i = 0; i < num_iter; i++) {
      uint64_t *addr =
          (uint64_t *)(base_addr + (i % 32) * 0x2000 + (i / 32) * 0x40);
      uint64_t val = *addr;
      sum = sum + val;
    }
  }

  // End marker
  END_PHASE_MARKER;

  // Put minion ID and sum into output buffer
  out_data[0] = minion_id;
  out_data[CACHE_LINE_SIZE] = sum;
  return 0;
}
