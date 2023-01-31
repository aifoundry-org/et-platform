/*
  Example of using cache_ops_evict_va api.
  Below is an example which demonstrates use of cache flush eviction.
*/

/* Include api specific header */
#include <etsoc/isa/cacheops-umode.h>
#include "utils.h"

/* Define base addresses for memory operations to generate shire cache and memory shire events.
   64 bytes are used to generate an event. */

#define MS_BASE_DEST     0x8100006000ULL
#define MS_BASE_SRC      0x8100009000ULL
#define TEST_DATA_SIZE   64
#define CACHE_LINE_BYTES 64

int main(void)
{
    /* Do a memory read/write. */
    et_memcpy(MS_BASE_DEST, MS_BASE_SRC, TEST_DATA_SIZE);

    /* Evict specified virtual address upto desired level */
    cache_ops_evict_va(0, to_Mem, MS_BASE_DEST, TEST_DATA_SIZE, 64, 0);

    return 0;
}