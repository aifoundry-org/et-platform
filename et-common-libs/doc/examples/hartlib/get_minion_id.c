/*
  Example of using get_minion_id api
  Below is an example which demonstrates how to print minion id. Minion id can be 
  retrieved using get_minion_id api and printed using et_printf api.
*/

/* Include api specific header */
#include <etsoc/isa/hart.h>
#include "utils.h"

int main(void)
{
    /* Check for specific minion id */
    if (get_minion_id() != 0)
    {
        /* Print information */
        et_printf("Running on Minion ID %d\n", get_minion_id());
    }

    return 0;
}