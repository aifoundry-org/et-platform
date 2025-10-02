#include <stdint.h>

int64_t entry_point(void);

/* Exception - useful for testing kernel exception handling of firmware */
int64_t entry_point(void)
{
    /* Generate a user mode exception */
    *(volatile uint64_t *)0 = 0xDEADBEEF;

    return 0;
}
