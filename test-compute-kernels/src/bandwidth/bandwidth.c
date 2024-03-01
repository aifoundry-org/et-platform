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

#define PER_HART_MEMORY_ALLOC 0x400000ULL
#define CACHE_LINE_SIZE 64
#define PMCS_DUMP_HART 62

typedef struct {
    uint64_t start_address;
    uint64_t end_address;
} AddressRange;

typedef struct {
  uint64_t base_addr;
  uint64_t num_minions;
  uint64_t num_cache_lines;
} Parameters;

int64_t entry_point(const Parameters*);

static int isAddressInRange(uint64_t address, AddressRange *range) {
    return (address >= range->start_address) &&
           (address <= range->end_address);
}

int64_t entry_point(const Parameters *const kernel_params_ptr)
{
  AddressRange hart_memory_range;

  // To be able enabled for debug only
  //et_printf("Hart[%d]:Kernel Param:base_addr:%ld\r\n", get_hart_id(), kernel_params_ptr->base_addr);
  //et_printf("Hart[%d]:Kernel Param:num_minions:%ld\r\n", get_hart_id(), kernel_params_ptr->num_minions);
  //et_printf("Hart[%d]:Kernel Param:num_cache_lines:%ld\r\n", get_hart_id(), kernel_params_ptr->num_cache_lines);

  uint32_t hart_id = get_hart_id();
  uint64_t value = 0;

  /* Assuming kernel_params_ptr is properly initialized
   Run 2 scenarios
  1) Only run on first hart from each Shire
  if (hart_id % 64 != 0)
  {
  	  return 0
  }

  2) Only run on first hart from each Neigh
  if (hart_id % 16 != 0)
  {
  	  return 0
  }
   */

  hart_memory_range.start_address = kernel_params_ptr->base_addr + hart_id * PER_HART_MEMORY_ALLOC;
  hart_memory_range.end_address   = hart_memory_range.start_address + PER_HART_MEMORY_ALLOC;

  et_trace_pmc_compute(PMCS_DUMP_HART);
  et_trace_pmc_sc(PMCS_DUMP_HART);
  et_trace_pmc_ms(PMCS_DUMP_HART, 0);
  et_trace_pmc_ms(PMCS_DUMP_HART, 1);
  et_trace_pmc_ms(PMCS_DUMP_HART, 2);
  et_trace_pmc_ms(PMCS_DUMP_HART, 3);
  et_trace_pmc_ms(PMCS_DUMP_HART, 4);
  et_trace_pmc_ms(PMCS_DUMP_HART, 5);
  et_trace_pmc_ms(PMCS_DUMP_HART, 6);
  et_trace_pmc_ms(PMCS_DUMP_HART, 7);

  uint64_t start_ts = et_get_timestamp();
  /* Note to extract SC and DDR Perf counters during the following loop execution */
  for (uint64_t addr = hart_memory_range.start_address; addr < hart_memory_range.end_address; addr += CACHE_LINE_SIZE) {
       value = *((uint64_t*)addr);
  }
  uint64_t delta_us = et_get_delta_timestamp(start_ts);

  et_trace_pmc_compute(PMCS_DUMP_HART);
  et_trace_pmc_sc(PMCS_DUMP_HART);
  et_trace_pmc_ms(PMCS_DUMP_HART, 0);
  et_trace_pmc_ms(PMCS_DUMP_HART, 1);
  et_trace_pmc_ms(PMCS_DUMP_HART, 2);
  et_trace_pmc_ms(PMCS_DUMP_HART, 3);
  et_trace_pmc_ms(PMCS_DUMP_HART, 4);
  et_trace_pmc_ms(PMCS_DUMP_HART, 5);
  et_trace_pmc_ms(PMCS_DUMP_HART, 6);
  et_trace_pmc_ms(PMCS_DUMP_HART, 7);

  uint64_t bytes_read = (PER_HART_MEMORY_ALLOC / CACHE_LINE_SIZE) * sizeof(value);
  et_printf("H: %u Kernel exec dur: %lu Measured B/W: %lu MB/s \n", hart_id, delta_us, (bytes_read / delta_us));

  return 0;
}
