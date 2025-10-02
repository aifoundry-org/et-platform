/*
  Example of using cache_ops_evict_va api.
  Below is an example which demonstrates how to.
*/

/* Include api specific header */
#include <etsoc/isa/cacheops-umode.h>

int main(void)
{
    /* Update ucache control */
    /* Enable scratchpad=1, cacheop rate=2 cycles, max cacheop=No limit*/
    cache_ops_ucache_control(1, 1, 0);

    /* Disable scratchpad=1, cacheop rate=4 cycles, max cacheop=1 outstanding request*/
    cache_ops_ucache_control(0, 2, 1);

    return 0;
}