#include <stdint.h>

// Hangs - useful for testing kernel abort
int64_t main(void)
{
    while(1);

    return 0;
}
