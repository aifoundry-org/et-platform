#include <stdint.h>
#include "hart.h"

// Exception and then hangs - useful for testing kernel exception handling of firmware
int64_t main(void)
{
    if ((get_shire_id() == 0) && ((get_minion_id() & 0x1f) == 0) && (get_thread_id() == 0)) {
        *(volatile uint64_t *)0 = 0xDEADBEEF;
    }

    while (1) {
        // Do nothing
    }

    return 0;
}
