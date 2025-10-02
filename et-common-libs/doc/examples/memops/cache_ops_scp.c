/*
  Example of using cache_ops_scp api.
  Below is an example which demonstrates how to enable scratchpad.
*/

/* Include api specific header */
#include <etsoc/isa/cacheops-umode.h>

int main(void)
{
    /* Enable scratchpad using function call*/
    cache_ops_scp(0x1, 0x1);

    return 0;
}