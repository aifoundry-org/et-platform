#include <stdint.h>
#include "etsoc/isa/hart.h"
#include "etsoc/common/utils.h"

/* Combine kernel - Generates multiple kernel execution error from different threads */
int64_t main(void)
{
    const uint64_t hart_id = get_hart_id() & ((SOC_MINIONS_PER_SHIRE * 2) - 1);
    int64_t ret = 0;
    switch(hart_id)
    {
        case 0 ... 7:
            /* Generate a user mode exception */
            *(volatile uint64_t *)0 = 0xDEADBEEF;
        break;

        case 8 ... 15:
            /* Hang the kernel for given HART */
            while(1);
        break;

        case 16 ... 23:
            /* Abort the kernel execution */
            et_abort();
        break;

        case 24 ... 31:
            ret = -10;
        break;

        default:
            return ret;

    }
    return ret;
}
