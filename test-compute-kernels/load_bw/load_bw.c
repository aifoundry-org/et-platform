
#include <stdint.h>
#include <stddef.h>
#include "hart.h"
#include "macros.h"
#include "common.h"
#include "kernel_params.h"
#include "log.h"

#define CACHE_LINE_SIZE 8

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

    // The number of iterations is 
    uint64_t hart_id = get_hart_id();
    uint64_t minion_id = hart_id >> 1;
  
    if (hart_id & 1) {     
        return 0;
    }

    uint64_t array_size = (uint64_t) kernel_params_ptr->tensor_b;
    uint64_t num_minions = (uint64_t) kernel_params_ptr->tensor_c;
    uint64_t num_iter = array_size / num_minions;
    volatile uint64_t *out_data = (uint64_t*) kernel_params_ptr->tensor_d;

    if (num_minions > 1024) {
      log_write(LOG_LEVEL_CRITICAL, "Number of minions should be <= 1024"); 
      return -1;
    }

    if (minion_id > num_minions) {
	return 0;
    }

    uint64_t sum = 0;

    // Start marker
    __asm__ __volatile__("slti x0,x0,0xaa");

    // num_iter is the same as the number of addresses each minion will access
    for (uint64_t i = 0; i < num_iter; i++) {
        uint64_t offset = (minion_id*num_iter) + i;

        uint64_t *offset_addr = (uint64_t *) (kernel_params_ptr->tensor_a + offset*8);
	uint64_t *final_addr = (uint64_t *) *offset_addr;
	uint64_t val1 = *final_addr;
	sum = sum + val1; // + val2;
    }

    // End marker
    __asm__ __volatile__("slti x0,x0,0xab");
    
    // Put minion ID and sum into output buffer
    out_data[CACHE_LINE_SIZE * minion_id] = minion_id;
    out_data[CACHE_LINE_SIZE * minion_id + 1] = sum;
    return 0;
}
