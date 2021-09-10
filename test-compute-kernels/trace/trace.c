#include <stdint.h>
#include <stddef.h>
#include "etsoc/common/utils.h"

int64_t main(void)
{
    et_printf("Hello from CM UMode.\n\r");
    et_printf("Now you can Debug me using Trace (~_^).\n\r");

    return 0;
}
