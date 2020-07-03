
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

// Prefetch test
// Tries to reach max BW in the memshire
int64_t main(const kernel_params_t* const kernel_params_ptr)
{

    if ((kernel_params_ptr == NULL) ||
        ((uint64_t*)kernel_params_ptr->tensor_a == NULL) ||
        (kernel_params_ptr->tensor_b == 0) ||
	(kernel_params_ptr->tensor_c == 0) ||
        (kernel_params_ptr->tensor_d == 0) ||
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
    uint64_t num_columns = (uint64_t) kernel_params_ptr->tensor_d;
    uint64_t num_iter = array_size / (num_columns * CACHE_LINE_SIZE * 8 * num_minions);
    volatile uint64_t *out_data = (uint64_t*) (kernel_params_ptr->tensor_e);

    if (num_minions > 1024) {
      log_write(LOG_LEVEL_CRITICAL, "Number of minions should be <= 1024"); 
      return -1;
    }

    // Sync up minions across shires.
    // Minion with minion_id = shire_id sends credit to all other shires to minions with same minion_id.
    uint64_t local_minion_id = minion_id & 0x1F;
    uint64_t target_min_mask = 1ULL << local_minion_id;
    uint64_t shire_id = minion_id >> 5;
    if (local_minion_id == shire_id) {
      for (uint64_t target_shire=0; target_shire < 32; target_shire++) {
        if (shire_id == target_shire) continue;
        SEND_FCC(target_shire, 0, 0, target_min_mask);
      }
    } else {
      WAIT_FCC(0);
    }

    // Synchronize all minions in shires now
    uint64_t barrier_result;
    WAIT_FLB(32, FCC_FLB, barrier_result);
    if (barrier_result == 1) {
      target_min_mask = 0xFFFFFFFFUL;
      target_min_mask = target_min_mask & (~(1ULL << minion_id));
      SEND_FCC(shire_id, 0, 1, target_min_mask);
    } else {
      WAIT_FCC(1);
    }
    FENCE;


    if ((minion_id % (1024 / num_minions)) != 0) {   // was % 4
	return 0;
    }
    
    // Phase 1 -- evict input tensor and laod clean lines into L3
    __asm__ __volatile__("slti x0,x0,0xaa");
  
    // Prefetch area of memory specified by tensr_a argument.
    // bank_ms_mc_offset: There are 128 bank/ms/mc groups. Each with 32 lines / row
    //                    Based on minion id minions are mapped to a bank/ms/mc.
    //                    For example minions 0-7 (if participating all) go to bank/mc/ms = 0
    // start_row_offset: For each iteration, what is the base address for all minions.
    // minion_row_offset: Within the row, on which column does the minion start fetching
    //                    1024 minions: 0x8000 x minion_id
    //                    512 minions: 0x10000 x (minion_id / 2)
    //                    256 minions: 0x20000 x (minion_id / 4)
    //                    128 minions: 0x40000 x (minion_id / 8)
    // num_iter: Equal to array_size / total amount fetched per iteration by all minions
    for (uint64_t i = 0; i < num_iter; i++) {
      uint64_t start_row_offset = 0x20000 * (num_minions / 8) * i; // divide num_minions by 8 or something ?
      uint64_t minion_row_offset = (array_size / num_minions) * (minion_id / (1024 / num_minions));
      uint64_t bank_mc_ms_offset = (minion_id / 8) * 0x40;
      uint64_t final_addr = base_addr + start_row_offset + minion_row_offset + bank_mc_ms_offset;
      prefetch_va(0, to_L1, final_addr, num_columns - 1, 0x2000, 0, 0);
      //if (minion_id == 0 || minion_id == 4 || minion_id == 8 || minion_id == 16) {
      // 	log_write(LOG_LEVEL_CRITICAL, "Prefetching 0x%lx\n", final_addr); 
      //}
    }

    WAIT_CACHEOPS;
    __asm__ __volatile__("slti x0,x0,0xab");

    // Put minion ID and sum into output buffer
    out_data[CACHE_LINE_SIZE * minion_id] = minion_id;
    return 0;
}
