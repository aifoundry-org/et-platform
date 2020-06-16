
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

// Load BW test.
// The test receives as input an array of addresses
// Each array is split into 1K parts, one for each minion
// Each minion accesses its own chunk and loads from these addresses

// Tensor A holds array of addresses
// Tensor B holds the size of Tensor A
// Tensor C holds the number of minions
// The number of iterations is Tensor B / Tensor C
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

    uint64_t clean_addr_base = 0x8180000000;
    uint64_t sum = 0;
    
    // Phase 1 -- evict input tensor and laod clean lines into L3
    __asm__ __volatile__("slti x0,x0,0xaa");
  
    // Evict input tensor
    for (uint64_t i = 0; i < num_iter; i++) {
        uint64_t offset = (minion_id*num_iter) + i;
        uint64_t offset_addr = kernel_params_ptr->tensor_a + offset*8;
	evict_va(0, to_Mem, offset_addr, 0, 0x40, 0, 0);
    }
    WAIT_CACHEOPS;

    // Now load clean lines so you evict dirty lines.
    // In total bring in 4MB x 32 = 128MB
    for (uint64_t i=0; i < 0x800; i++) {
      uint64_t *clean_addr = (uint64_t *) (clean_addr_base + minion_id * 0x20000 + i * 64);
      uint64_t content = *clean_addr;
      sum = sum + content;
    }
   
    // Prefetch tensor into L2
    for (uint64_t i = 0; i < num_iter; i++) {
        uint64_t offset = (minion_id*num_iter) + i;
	uint64_t offset_addr = kernel_params_ptr->tensor_a + offset*8;
	prefetch_va(0, 1, offset_addr, 0, 0x40, 0, 0);
    }

    WAIT_CACHEOPS;

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

    __asm__ __volatile__("slti x0,x0,0xab");


    // Phase 2 -- this is the actual load_bw test:
    // Fetch pointers from tensor a and load their contents
    __asm__ __volatile__("slti x0,x0,0xaa");

    // num_iter is the same as the number of addresses each minion will access
    for (uint64_t i = 0; i < num_iter; i++) {
        uint64_t offset = (minion_id*num_iter) + i;

        uint64_t *offset_addr = (uint64_t *) (kernel_params_ptr->tensor_a + offset*8);
	uint64_t *final_addr = (uint64_t *) *offset_addr;
	uint64_t val = *final_addr;
	sum = sum + val;
    }

    // End marker
    __asm__ __volatile__("slti x0,x0,0xab");
    
    // Put minion ID and sum into output buffer
    out_data[CACHE_LINE_SIZE * minion_id] = minion_id;
    out_data[CACHE_LINE_SIZE * minion_id + 1] = sum;
    return 0;
}
