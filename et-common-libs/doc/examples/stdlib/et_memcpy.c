/*
  Examples of using et_memcpy api
  Below is an example which shows usage of et_memcpy by copying from one array to other with
  complete size and then only 4 bytes. It also shows how data structure can be copied from
  one structure to other.
*/

/* Include api specific header */
#include "utils.h"

/* Define a simple structure for example usage */
typedef struct {
    int foo;
    uint8_t bar;
} sample_config_t;

int main(void)
{
    /* Define two example arrays */
    uint8_t src[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    uint8_t dst[8];

    /* Define two example structures */
    sample_config_t src_config = { 123, 0xA };
    sample_config_t dst_config;

    /* Copy complete source buffer int dst */
    et_memcpy(dst, src, sizeof(src));

    /* Copy 4 bytes from source buffer int dst */
    et_memcpy(dst, src, 4);

    /* Using et_memcpy to copy structure */
    et_memcpy(&dst_config, &src_config, sizeof(src_config));

    return 0;
}