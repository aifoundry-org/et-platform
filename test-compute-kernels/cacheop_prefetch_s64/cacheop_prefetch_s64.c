#include "etsoc/isa/hart.h"
#include "etsoc/isa/cacheops.h"
#include "etsoc/common/utils.h"
#include "common.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// tensor_a indicates destination of the prefetch operation. Should be 1 or 2.
// This test prefetches on thread 1 of each of the minion
// Stride is 64, and with 16 lines,  all the mem-shires will be accesssed by each minion
typedef struct {
  uint64_t dst;
} Parameters;
int64_t main(const Parameters* const kernel_params_ptr)
{

    if ((kernel_params_ptr == NULL))
    {
        // Bad arguments
        et_printf("Programming returing due to error\n");
        return -1;
    }

    uint64_t hart_id = get_hart_id();
    uint64_t shire_id = ((hart_id>>6) & 0x3F);
    uint64_t minion_id = ((hart_id>>1) & 0x1F);
    uint64_t N_TIMES = 100;
    uint64_t dst = kernel_params_ptr->dst; // 1 or 2. Destination of the prefetch

if ((hart_id & 1) == 1) //Only Thread1 (or prefetch threads to do prefetch)
   {
	   for(uint64_t i = 0; i < N_TIMES ; i++) //N_TIMES = # of times the thread1 performs the prefetch CSR
	   {
		   volatile uint64_t VA = (uint64_t)(((0x82ULL)<<32) + ((0xaU)<<28U) + (shire_id << 23) + (minion_id << 18) + ((0x01*i)<<10) + (0x0 << 6));

		   prefetch_va(false,     dst,   (uint64_t)(VA),  15,         64,      0);
		   //WAIT_PREFETCH_0;
	   }

           return 0;
   }
   else {return 0;}
}
