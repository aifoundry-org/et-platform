/*
  Examples of using et_memcmp api
  Below is an example which shows usage of et_memcmp by comparing two data structures
  and strings. Prints the results based on comparrison.
*/

/* Include api specific header */
#include "utils.h"

/* define a simple structure for example usage */
typedef struct {
    int foo;
    uint8_t bar;
} sample_config_t;

int main(void)
{
    /* Define two example structures */
    sample_config_t src_config = { 123, 0xA };
    sample_config_t dst_config;

    /* Define two example arrays */
    char str_1[10] = "Test str";
    char str_2[10] = "Test";

    /* Using et_memcmp to compare data structures */
    if (et_memcmp(&dst_config, &src_config, sizeof(src_config)) == 0)
    {
        et_printf("dst_config is equal to rc_config");
    }

    /* Using et_memcmp to compare two strings*/
    if (et_memcmp(&str_1, &str_2, sizeof(src_config)) == 0)
    {
        /* In case where strings are equal */
        et_printf("str_1 is equal to str_2");
    }
    else
    {
        /* Strings are not equal */
        et_printf("str_1 is not equal to str_2");
    }

    return 0;
}