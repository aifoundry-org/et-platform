/*
  Examples of using et_get_delta_timestamp api
  Below is an example which demonstrates using et_get_timestamp. It calculates latency of a function call.
*/

/* Include api specific header */
#include "utils.h"

int main(void)
{
    /* Keep record of start time stamp */
    uint64_t start_timestamp = et_get_timestamp();
    int i = 0;

    /* wait for some time */
    while (i++ < 10000)
        ;

    /* Calculate cycles consumed */
    uint64_t elapsed_time_us = (et_get_timestamp() - start_timestamp);

    /* Print cycles consumed in operation */
    et_printf("elapsed cycles : %ld", elapsed_time_us);

    return 0;
}