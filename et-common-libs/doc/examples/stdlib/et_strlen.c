/*! \file et_strlen.c
  Examples of using et_strlen api
  Below is an example which demonstrates using strlen. It prints the string
  length of an example string, also it copies string from one array to other
  with size of string length.
*/

/* Include api specific header */
#include "utils.h"

int main(void)
{
    /* Define two example arrays. */
    char ptr[10];
    char data[5] = "test\0";

    /* Get the string length of data array. */
    int length = et_strlen(data);

    /* printing string length using et_strlen */
    et_printf("Length of string - %s is: %d", data, length);

    /* copy one string to other using strlen */
    et_memcpy(ptr, data, et_strlen(data));

    return 0;
}