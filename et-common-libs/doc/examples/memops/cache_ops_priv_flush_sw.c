/*
  Below is an example which demonstrates how to use cache_ops_priv_flush_sw
  to different levels of cache, with different set, way and thread mask params.
*/

/* Include api specific header */
#include <etsoc/isa/cacheops-umode.h>

int main(void)
{
    int32_t status;

    /* use_tmask=0, dst=1 (L2/SP_RAM), set=0, way=0, num_lines=5 */
    status = cache_ops_priv_flush_sw(0, to_L2, 0, 0, 5);

    /* Check if eviction was successfull */
    et_assert(status == 0);

    /* use_tmask=1, dst=1 (L2/SP_RAM), set=2, way=4, num_lines=15 */
    status = cache_ops_priv_flush_sw(1, to_L3, 2, 4, 15);

    /* Check if eviction was successfull */
    et_assert(status == 0);

    return 0;
}