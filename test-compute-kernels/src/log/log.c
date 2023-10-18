#include "etsoc/isa/hart.h"
#include "etsoc/common/utils.h"

#include <stdint.h>

int64_t entry_point(void)
{
    if (get_hart_id() == 42)
    {
        et_printf("hello world");
    }

    return 0;
}
