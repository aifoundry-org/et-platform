/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
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

#include <etsoc/common/utils.h>
#include <etsoc/isa/hart.h>
#include <trace/trace_umode.h>

#define CACHE_LINE_SIZE 64

typedef struct {
    uint64_t start_address;
    uint64_t end_address;
} AddressRange;

typedef struct {
  uint64_t base_addr;
  uint64_t per_hart_mem_alloc;
  uint64_t minion_freq;
} Parameters;

int64_t entry_point(const Parameters*);

static int isAddressInRange(uint64_t address, AddressRange *range) {
    return (address >= range->start_address) &&
           (address <= range->end_address);
}

int64_t entry_point(const Parameters *const kernel_params_ptr)
{
  AddressRange hart_memory_range;

  uint32_t hart_id = get_hart_id();
  volatile uint64_t value = 0;

  /* Assuming kernel_params_ptr is properly initialized
   Run 2 scenarios
  1) Only run on first hart from each Shire 
  if (hart_id % 64 != 0)
  {
  	  return 0;
  }
  2) Only run on first hart from each Neigh
  if (hart_id % 16 != 0)
  {
  	  return 0;
  }
 */

  uint64_t per_hart_mem_alloc = kernel_params_ptr->per_hart_mem_alloc;
  hart_memory_range.start_address = kernel_params_ptr->base_addr + hart_id * per_hart_mem_alloc;
  hart_memory_range.end_address   = hart_memory_range.start_address + per_hart_mem_alloc;

  // To avoid all Minions from accessing the same L3 shire at the same time randomize starting L3 slice
  // Use simple XOR shift algorithm for pseudo-random number generation
  uint16_t randSeed = hart_id ^ (hart_id << 5) ^ (hart_id >> 3);
  hart_memory_range.start_address += (randSeed % 8 * CACHE_LINE_SIZE);

  // Snapshot timestamp prior to execution
  uint64_t start_ts = et_get_timestamp();
  for (uint64_t addr = hart_memory_range.start_address; addr < hart_memory_range.end_address; addr += CACHE_LINE_SIZE) {
       value = *((uint64_t*)addr);
  }
  // Snapshot timestamp upon completion of execution
  uint64_t dur_cycles = et_get_delta_timestamp(start_ts);

  // Convert to time and print bandwidth
  double dur_time = dur_cycles / kernel_params_ptr->minion_freq ;
  et_printf("Measured B/W: %0.3f MB/s (Cycles: %lu) \n", (per_hart_mem_alloc / dur_time), dur_cycles);

  return 0;
}
