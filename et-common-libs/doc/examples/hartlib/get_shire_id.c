/*
  Example of using get_shire_id api
  Below is an example which demonstrates how to do a specific task on a particular 
  shire id, also how to calculate thread count based on shire id. 
*/

/* Include api specific header */
#include <etsoc/isa/hart.h>
#include "utils.h"

/* Example define of master shire id */
#define MASTER_SHIRE 32

int main(void)
{
    uint32_t thread_count = 0;
    int shire_id = get_shire_id();

    /* Print shire ID */
    et_printf("shire ID", shire_id);

    if (get_shire_id() == 0)
    {
        /* Return if shire ID is 0 */
        return 0;
    }

    /* Populate thread count based on shire ID */
    if (shire_id == MASTER_SHIRE)
    {
        thread_count = 32;
    }
    else
    {
        thread_count = 64;
    }

    return 0;
}