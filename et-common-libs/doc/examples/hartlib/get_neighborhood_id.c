/*
  Example of using get_neighborhood_id api
  Below is an example which demonstrates how to print neighborhood id. Neighborhood id can be 
  retrieved using get_neighborhood_id api and printed using et_printf api.
*/

/* Include api specific header */
#include <etsoc/isa/hart.h>
#include "utils.h"

int main(void)
{
    /* Check for specific neighborhood id */
    if (get_neighborhood_id() != 0)
    {
        /* Print information */
        et_printf("Running on Neighborhood ID %d\n", get_neighborhood_id());
    }

    return 0;
}