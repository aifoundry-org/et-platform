/*
  Example of using cache_ops_priv_evict_l1 api.
  Below is an example which demonstrates how to invalidate L1 cache upto specified level.
*/

/* Include api specific header */
#include <etsoc/isa/cacheops-umode.h>
#include "utils.h"

int main(void)
{
    int32_t status;

    /* Invalidate L1 to desired L2 level */
    status = cache_ops_priv_evict_l1(0, to_L2);

    /* Check if api call was successful*/
    et_assert(status == 0)

        return 0;
}