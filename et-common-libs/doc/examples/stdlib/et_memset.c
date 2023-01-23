/*
  Examples of using et_memset api
  Below is an example which shows usage of et_memset api by setting a buffer to 
  null character, a specific character # and hex bytes with value 0xA5.
*/
/* Include api specific header */
#include "utils.h"

int main(void)
{
    uint8_t buffer[8];

    /* Fill the memory block with null bytes */
    et_memset(buffer, '\0', 8);

    /* Fill the memory block with hashes */
    et_memset(buffer, '#', 8);

    /* Fill the memory block with hex value */
    et_memset(buffer, 0xA5, 8);

    return 0;
}