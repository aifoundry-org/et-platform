/*
  Example of using cache_ops_priv_cache_invalidate api.
  Below is an example which demonstrates how to.
*/

/* Include api specific header */
#include <etsoc/isa/cacheops-umode.h>
#include "utils.h"

int main(void)
{
    int32_t status;

    /* Invalidate both TLB, PTW and instructions cache */
    status = cache_ops_priv_cache_invalidate(1, 0);

    /* Check if api call was successful*/
    et_assert(status == 0)

        return 0;
}