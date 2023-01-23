/*! \file et_get_timestamp.c
  Examples of using et_get_timestamp api
  Below is an example which demonstrates using et_get_timestamp. It prints current
  timestamp and shows a timeout implementation using it.
  with size of string length.
*/

/* Include api specific header */
#include "utils.h"

int main(void)
{
    /* printing string length using et_strlen */
    et_printf("Current cycles : %ld", et_get_timestamp());

    /* Define a cycles value which is 1000 more then current cycles */
    uint64_t cycles = et_get_timestamp() + 1000;

    /* do iterations until timeout */
    while (et_get_timestamp() < cycles)
    {
        /* do some operation and Wait for timeout */
    }

    return 0;
}