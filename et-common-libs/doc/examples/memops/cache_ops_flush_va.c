/*
  Example of using cache_ops_flush_va api.
  Below is an example which demonstrates memory eviction. A memory read/write operation will be 
  performed and then data will be evicted using api.
*/

/* Include api specific header */
#include <etsoc/isa/cacheops-umode.h>
#include "utils.h"

/* Define memory address and data size */
#define MS_BASE_DEST   0x8100006000ULL
#define TEST_DATA_SIZE 64

int main(void)
{
    /* Flush specified virtual address upto desired level */
    cache_ops_flush_va(0, to_L2, MS_BASE_DEST, TEST_DATA_SIZE, 64, 0);

    return 0;
}