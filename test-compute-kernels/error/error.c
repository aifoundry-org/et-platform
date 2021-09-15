#include <stdint.h>

/* User error - useful for testing kernel user error handling of firmware */
int64_t main(void)
{
    /* Generate a user error */
    return -10;
}
