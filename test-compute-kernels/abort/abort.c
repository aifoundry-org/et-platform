#include <stdint.h>
#include "etsoc/common/utils.h"

/* Self abort kernel */
int64_t main(void)
{
    /* Dummy execution */
    int temp = 0;
    temp++;

    /* Abort the kernel execution */
    et_abort();

    return 0;
}
