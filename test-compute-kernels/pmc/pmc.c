#include <stdint.h>
#include <stddef.h>
#include <etsoc/common/utils.h>
#include <trace/trace_umode.h>
#include <etsoc/isa/cacheops-umode.h>
#include "etsoc/isa/hart.h"

int64_t main(void)
{
    uint64_t hart_id = get_hart_id();

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

        /* PMC Shire Cache test for L2_MISS_REQ, PMU_SC_L2_READS, and ICACHE_ETLINK_REQ:
           Read from U-mode range memory address, which is not in L1 cache.
           L2_MISS_REQ, PMU_SC_L2_READS, and ICACHE_ETLINK_REQ should at least be incremented by one. */
        et_printf("PMC Shire Cache, L2_MISS_REQ, and ICACHE_ETLINK_REQ.\n\r");
        et_trace_pmc_memory(hart_id);
        et_trace_pmc_compute(hart_id);
        et_memcpy((uint64_t*)0x8102000000, (uint64_t*)0x8102001000, 64);
        et_trace_pmc_compute(hart_id);
        et_trace_pmc_memory(hart_id);

        /* Do a memory read/write.
           TODO: Mem shire events needs to be validated after SW-10308. */
        et_printf("PMC Mem Shire events.\n\r");
        et_memcpy((uint64_t*)0x8102002000, (uint64_t*)0x8102003000, 64);
        et_trace_pmc_memory(hart_id);
        et_trace_pmc_compute(hart_id);
        cache_ops_evict(to_Mem, (uint64_t*)0x8102000000, 64);
        et_trace_pmc_compute(hart_id);
        et_trace_pmc_memory(hart_id);
    }

    return 0;
}
