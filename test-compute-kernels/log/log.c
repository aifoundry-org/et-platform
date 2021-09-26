#include "etsoc/isa/hart.h"
#include "log.h"

#include <stdint.h>

int64_t main(void)
{
    if (get_hart_id() == 42)
    {
        log_write(LOG_LEVEL_CRITICAL, "hello world");
    }

    return 0;
}
