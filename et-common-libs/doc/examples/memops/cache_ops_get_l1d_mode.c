/*
  Example of using cache_ops_evict_va api.
  Below is an example which demonstrates how to get cache l1d mode.
*/

/* Include api specific header */
#include <etsoc/isa/cacheops-umode.h>
#include "utils.h"

int main(void)
{
    /* Obtain l1d mode */ * / enum l1d_mode = cache_ops_get_l1d_mode();

    /* Print l1d mode on console*/
    switch (l1d_mode)
    {
        case l1d_shared:
            et_printf("L1 Cache mode is Shared\n");
            break;
        case l1d_split:
            et_printf("L1 Cache mode is Split\n");
            break;
        case l1d_scp:
            et_printf("L1 Cache mode is SCP\n");
            break;
        default:
            et_printf("Invalid L1 Cache mode\n");
            break;
    }

    return 0;
}