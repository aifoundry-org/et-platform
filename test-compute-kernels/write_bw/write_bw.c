
#include <stdint.h>
#include <stddef.h>
#include "hart.h"
#include "macros.h"
#include "common.h"
#include "kernel_params.h"
#include "log.h"
#include "cacheops.h"
#include "fcc.h"
#include "flb.h"

#define CACHE_LINE_SIZE 8
#define FCC_FLB 2

// Write BW test.
// Evictions...
int64_t main(const kernel_params_t* const kernel_params_ptr)
{

    if ((kernel_params_ptr == NULL) ||
        ((uint64_t*)kernel_params_ptr->tensor_a == NULL) ||
        (kernel_params_ptr->tensor_b == 0) ||
	(kernel_params_ptr->tensor_c == 0) ||
	(uint64_t*)kernel_params_ptr->tensor_d == NULL)
    {
        // Bad arguments
        return -1;
    }

    uint64_t hart_id = get_hart_id();
    uint64_t minion_id = hart_id >> 1;  
    if (hart_id & 1) {
        return 0;
    }

    // Set marker for waveforms
    __asm__ __volatile("slti x0,x0,0xfb");
 
    uint64_t base_addr = (uint64_t) kernel_params_ptr->tensor_a;
    uint64_t array_size = (uint64_t) kernel_params_ptr->tensor_b;
    uint64_t num_minions = (uint64_t) kernel_params_ptr->tensor_c;
    uint64_t num_iter = array_size / num_minions;
    volatile uint64_t *out_data = (uint64_t*) (kernel_params_ptr->tensor_d);

    if (num_minions > 1024) {
      log_write(LOG_LEVEL_CRITICAL, "Number of minions should be <= 1024"); 
      return -1;
    }

    if (minion_id > num_minions) {
	return 0;
    }
    
    // Phase 1 -- evict input tensor and laod clean lines into L3
    __asm__ __volatile__("slti x0,x0,0xaa");
  
    // Evict input tensor -- it should all be in L3.
    // 32 MB total: 32KB per minion, so 512 lines.
    for (uint64_t i = 0; i < num_iter; i++) {
      uint64_t offset = minion_id * num_iter * 0x40 + i * 0x40;
      uint64_t final_addr = base_addr + offset;
      evict_va(0, to_Mem, final_addr, 0, 0x40, 0, 0);
    }
    WAIT_CACHEOPS;
    __asm__ __volatile__("slti x0,x0,0xab");

    // Put minion ID and sum into output buffer
    out_data[CACHE_LINE_SIZE * minion_id] = minion_id;
    return 0;
}
