/*
  Example of using cache_ops_priv_l1_cache_lock_sw api.
  Below is an example which demonstrates how to.
*/

/* Include api specific header */
#include <etsoc/isa/cacheops-umode.h>

int main(void)
{
    int32_t status;

    /* Hard lock the given address with way value as 0*/
    status = cache_ops_priv_l1_cache_lock_sw(0, 0x8102000000);

    /* check if status was successful*/
    et_assert(status == 0)

        return 0;
}