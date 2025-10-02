#include <stdint.h>

int64_t entry_point(void);

// Hangs - useful for testing kernel abort
int64_t entry_point(void)
{
    while(1);

    return 0;
}
