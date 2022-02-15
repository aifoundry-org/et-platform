#include <stdint.h>

/* Exception - useful for testing kernel exception handling of firmware */
int64_t main(void)
{
    /* Generate a user mode exception */
    *(volatile uint64_t *)0 = 0xDEADBEEF;

    return 0;
}
