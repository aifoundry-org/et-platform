
#include <stdint.h>
#include <stddef.h>
#include "hart.h"
#include "macros.h"
#include "common.h"
#include "kernel_params.h"
#include "log.h"
#include "cacheops.h"

#define CACHE_LINE_SIZE 8
#define FCC_FLB 2

// Loat Lat Test
// Sends a sequence of addresses from one minion only touching all MS/MC/banks
// Can serve as a latency test for the memory controller + DRAM but also for NOC
// and other parts of the system.

// Tensor A holds start address
// Tensor B holds the number of iterations (addresses) - 4K needed to cover all possible addresses in one row
// in all MS/MC/banks
// Tensor C holds the minion id that will send the loads
// Tensor D holds a flag that shows whether we do open or closed page accesses.
// For open page we need to traverse the columns in a single row first.
// For closed page we go through all MS/MC/banks first, and the open pages close based on a timer.
// Tensor E holds the output which should be the minion id that sent the loads
int64_t main(const kernel_params_t* const kernel_params_ptr)
{

    if ((kernel_params_ptr == NULL) ||
        (kernel_params_ptr->tensor_a == 0) ||
        (kernel_params_ptr->tensor_b == 0) ||
	(uint64_t*)kernel_params_ptr->tensor_e == NULL)
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
    uint64_t num_iter = (uint64_t) kernel_params_ptr->tensor_b;
    uint64_t active_minion_id = (uint64_t) kernel_params_ptr->tensor_c;
    bool open_page = ((uint64_t) kernel_params_ptr->tensor_d) == 1;
    volatile uint64_t *out_data = (uint64_t*) (kernel_params_ptr->tensor_e);

    if (active_minion_id > 1023) {
      log_write(LOG_LEVEL_CRITICAL, "Active minion id should be  < 1024"); 
      return -1;
    }

    if (active_minion_id != minion_id) {
	return 0;
    }
    
    __asm__ __volatile("slti x0,x0,0xaa");
    
    uint64_t sum = 0;
    // Closed page accesses.
    if (!open_page) {
      for (uint64_t i=0; i< num_iter; i++) {
	uint64_t *addr = (uint64_t *) (base_addr + i * 0x40);
	uint64_t val = *addr;
	sum = sum+val;
      }
    }
    // Open page accesses. Column offset is 0x2000. Consecutive request go to consecutive columns, same row
    // Then we switch to the same row of another MS/MC/bank.
    else {
       for (uint64_t i=0; i< num_iter; i++) {
	 uint64_t *addr = (uint64_t *) (base_addr + (i % 32) * 0x2000 + (i/32) * 0x40);
	 uint64_t val = *addr;
	 sum = sum+val;
      }
    }

    // End marker
    __asm__ __volatile__("slti x0,x0,0xab");
    
    // Put minion ID and sum into output buffer
    out_data[0] = minion_id;
    out_data[CACHE_LINE_SIZE] = sum;
    return 0;
}
