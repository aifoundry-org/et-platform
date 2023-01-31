/*
  Example of using cache_ops_prefetch_va api.
  Below is an example which demonstrates how to prefetch data to specified cache level.
*/

/* Include api specific header */
#include <etsoc/isa/cacheops-umode.h>

/* Define memory address and data size */
#define MS_BASE_DEST   0x8100006000ULL
#define TEST_DATA_SIZE 64

int main(void)
{
    /* Prefetch specified virtual address upto desired level */
    cache_ops_prefetch_va(0, to_L2, MS_BASE_DEST, TEST_DATA_SIZE, 64, 0);

    return 0;
}