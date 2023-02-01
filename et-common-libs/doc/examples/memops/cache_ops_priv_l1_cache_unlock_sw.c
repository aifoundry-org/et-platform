/*
  Example of using cache_ops_priv_l1_cache_unlock_sw api.
  Below is an example which demonstrates how to unlock D-cache in L1.
*/

/* Include api specific header */
#include <etsoc/isa/cacheops-umode.h>
#include "utils.h"

int main(void)
{
    int32_t status;

    /* Unlock L1 D-cache with way=0 and set=14*/
    status = cache_ops_priv_l1_cache_unlock_sw(0, 14);

    /* Check if api call was successful*/
    et_assert(status == 0)

        return 0;
}