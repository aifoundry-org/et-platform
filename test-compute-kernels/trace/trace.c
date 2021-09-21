#include <stdint.h>
#include <stddef.h>
#include "etsoc/common/utils.h"

int64_t main(void)
{
    char str[] = "Message";

    et_printf("Hello from CM UMode.\n\r");
    et_printf("Now you can Debug me using Trace (~_^).\n\r");
    et_printf("Test: String \"%s\" has length %d.\n\r", str, et_strlen(str));

    return 0;
}
