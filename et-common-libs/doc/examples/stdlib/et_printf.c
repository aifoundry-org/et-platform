/*! \file et_printf.c
  Examples of using et_printf api
  Below is an example which demonstrates how et_printf can be used to print different kind
  of variables with different format specifiers.
*/

/* Include api specific header */
#include "utils.h"

int main(void)
{
    uint8_t buffer[8];

    /* An example of printing integers. */
    et_printf("Integers: %i %u \n", -3456, 3456);

    /* An example of printing characters. */
    et_printf("Characters: %c %c \n", 'z', 80);

    /* An example of printing decimals. */
    et_printf("Decimals: %d %ld\n", 1997, 32000L);

    /* An example of printing radices. */
    et_printf("Some different radices: %d %x %o %#x %#o \n", 100, 100, 100, 100, 100);

    /* An example of printing floats. */
    et_printf("floats: %4.2f %+.0e %E \n", 3.14159, 3.14159, 3.14159);

    /* An example of printing string. */
    et_printf("String: %s \n", "Esperanto");

    return 0;
}