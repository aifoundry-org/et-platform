#include <stdint.h>
#include <stddef.h>
#include <etsoc/common/utils.h>
#include <trace/trace_umode.h>
#include <etsoc/isa/cacheops-umode.h>
#include <etsoc/isa/atomic.h>
#include "etsoc/isa/hart.h"

/* Define base addresses for memory operations to generate shire cache and memory shire events.
   Only first hart of each neighborhood is generating/logging events.
   64 bytes are used to generate an event.
   Buffer size is reserved to be > size per event * neigh count = 64 * 130. */
#define SC_BASE_DEST    0x8100000000ULL
#define SC_BASE_SRC     0x8100003000ULL
#define MS_BASE_DEST    0x8100006000ULL
#define MS_BASE_SRC     0x8100009000ULL
#define TEST_DATA_SIZE  64
#define MS_PMCS_DUMP_HART 0

#define GET_NEIGH_BASE(base, neigh_index)     (void *)(base + (neigh_index * TEST_DATA_SIZE))

int64_t entry_point(void)
{
    uint64_t hart_id = get_hart_id();
    uint64_t neigh_index = neigh_index = hart_id / 16;
    
    /* Run the test for first thread of each neighborhood. */
    if (hart_id % 16 == 0)
    {
        /* PMC compute test for RETIRED_INST0:
           When only HART0's events are enabled for one particular counter e.g. PMU_MHPMEVENT4.
           Then Expected delta across 'for loop' is  approx 60800.
           Since we are in for loop also expects the instruction cache hits. */
        et_printf("PMC compute test for RETIRED_INST0.\n\r");
        et_trace_pmc_compute(hart_id);
        for (size_t i = 0; i < 10000; i++)
        {
            asm volatile("addi x0, x0, 0");
        }
        et_trace_pmc_compute(hart_id);

        /* PMC Shire Cache test for L2_MISS_REQ, PMU_SC_L2_READS, PMU_SC_L2_WRITES,
           and ICACHE_ETLINK_REQ:
           DO atomic load and store from L2 cache from U-mode range memory address.
           L2_MISS_REQ, PMU_SC_L2_READS, PMU_SC_L2_WRITES, and ICACHE_ETLINK_REQ should at least be incremented by one. */
        et_printf("PMC Shire Cache, L2_MISS_REQ, and ICACHE_ETLINK_REQ.\n\r");
        et_trace_pmc_sc(hart_id);
        uint64_t data = atomic_load_local_64(GET_NEIGH_BASE(SC_BASE_SRC, neigh_index));
        data += 1;
        atomic_store_local_64(GET_NEIGH_BASE(SC_BASE_DEST, neigh_index), data);
        et_trace_pmc_sc(hart_id);
    }


      /* A single hart in a kernel launch will dump MS PMCs of all memshires
      before doing a memory request and after doing the memory request
      Both read and write PMCs should be incremented */
      et_trace_pmc_ms(MS_PMCS_DUMP_HART, 0);
      et_trace_pmc_ms(MS_PMCS_DUMP_HART, 1);
      et_trace_pmc_ms(MS_PMCS_DUMP_HART, 2);
      et_trace_pmc_ms(MS_PMCS_DUMP_HART, 3);
      et_trace_pmc_ms(MS_PMCS_DUMP_HART, 4);
      et_trace_pmc_ms(MS_PMCS_DUMP_HART, 5);
      et_trace_pmc_ms(MS_PMCS_DUMP_HART, 6);
      et_trace_pmc_ms(MS_PMCS_DUMP_HART, 7);

      if (hart_id == MS_PMCS_DUMP_HART)
      {
         /* Do a memory read/write. */
         et_printf("PMC Mem Shire events.\n\r");
         et_memcpy(GET_NEIGH_BASE(MS_BASE_DEST, neigh_index), GET_NEIGH_BASE(MS_BASE_SRC, neigh_index), TEST_DATA_SIZE);
         cache_ops_evict(to_Mem, GET_NEIGH_BASE(MS_BASE_DEST, neigh_index), TEST_DATA_SIZE);
      }
      
      et_trace_pmc_ms(MS_PMCS_DUMP_HART, 0);
      et_trace_pmc_ms(MS_PMCS_DUMP_HART, 1);
      et_trace_pmc_ms(MS_PMCS_DUMP_HART, 2);
      et_trace_pmc_ms(MS_PMCS_DUMP_HART, 3);
      et_trace_pmc_ms(MS_PMCS_DUMP_HART, 4);
      et_trace_pmc_ms(MS_PMCS_DUMP_HART, 5);
      et_trace_pmc_ms(MS_PMCS_DUMP_HART, 6);
      et_trace_pmc_ms(MS_PMCS_DUMP_HART, 7); 

    return 0;
}
