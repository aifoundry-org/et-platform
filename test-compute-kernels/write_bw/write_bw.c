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

// Write BW test.
// Not clearly an open / closed page test, however we should see
// memory controller stats to get BW.
// DMA loads 32MB array into L3.
// Then each minion evicts 32KB bytes to DDR. Minions are assigned sequential
// blocks.
// A problem that we have to generate predictable traffic is that we do not know
// where runtime
// will place input tensor. The algorithm will be different if it is placed on a
// row or column boundary
//
// Todo: Probably this tes should change to use stores / tensor stores and then
// evicts to DDR
typedef struct {
  uint64_t base_addr;
  uint64_t array_size;
  uint64_t num_minions;
  uint64_t *out_data;
}  Parameters;
int64_t main(const Parameters *const kernel_params_ptr) {
  if (kernel_params_ptr == NULL || kernel_params_ptr->base_addr == 0 ||
      kernel_params_ptr->array_size == 0 ||
      kernel_params_ptr->num_minions == 0 ||
      kernel_params_ptr->out_data == NULL) {
    // Bad arguments
    return -1;
  }

  uint64_t hart_id = get_hart_id();
  uint64_t minion_id = hart_id >> 1;

  // Have odd hart finish with even one to reduce reads to memory
  // Todo: Verify that this is important.
  if (hart_id & 1) {
    WAIT_FCC(0);
    return 0;
  }

  // Set marker for waveforms
  START_WAVES_MARKER;

  uint64_t base_addr = kernel_params_ptr->base_addr;
  uint64_t array_size = kernel_params_ptr->array_size;
  uint64_t num_minions = kernel_params_ptr->num_minions;
  uint64_t num_iter = array_size / (num_minions * 1024);
  volatile uint64_t *out_data = kernel_params_ptr->out_data;

  if (num_minions > 1024) {
    et_printf("Number of minions should be <= 1024");
    return -1;
  }

  if (minion_id > num_minions) {
    return 0;
  }

  // Sync up minions across shires.
  sync_up_all_minions(minion_id, 32, FCC_FLB);

  // Phase 1 -- evict input tensor and laod clean lines into L3
  START_PHASE_MARKER;

  // Delay loop for minions that try to evict addresses that are already out.
  // This avoids reads to the memshire after these minions finish their kernel.
  if (minion_id >= 928) {
    for (volatile uint64_t j = 0; j < 40000; j++)
      ;
  }

  // Evict input tensor -- it should all be in the L3
  // TODO: There is a big empty gap for MC3-MC15. The gap is when some minions
  // finish
  // Then activity resumes for minions that go slower. But some minions
  // apparently never stop activity ?
  // To finish when all minions finish you need to re-sync the minions.
  // In that way you will be able to remove the delay loop.
  // TODO: This is a mixed open-close pattern. To have only closed, set num
  // lines to 1
  // and have the last term minion_id * 0x40 (instead of 0x400)
  for (uint64_t i = 0; i < num_iter; i++) {
    uint64_t final_addr =
        base_addr + (i * num_minions) * 0x400 + minion_id * 0x400;
    evict_va(0, to_Mem, final_addr, 15, 0x40, 0);
  }

  WAIT_CACHEOPS;

  END_PHASE_MARKER;

  // Unblock thread 1
  uint64_t shire_id = (minion_id >> 5) & 0x1F;
  uint64_t local_minion_id = minion_id & 0x1F;
  SEND_FCC(shire_id, 1, 0, (1ULL << local_minion_id));

  // Put minion ID and sum into output buffer
  out_data[CACHE_LINE_SIZE * minion_id] = minion_id;

  return 0;
}
