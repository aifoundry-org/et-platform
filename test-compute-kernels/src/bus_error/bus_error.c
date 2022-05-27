#include <stdint.h>
#include <etsoc/isa/cacheops-umode.h>
#include <etsoc/isa/hart.h>

/* Generates bus error */
int main(void)
{
    /* Generate bus error on single hart */
    if (get_hart_id() == 0)
    {
        cache_ops_prefetch_va(0, 2, 0x80000000, 16, 0x40, 0);
    }

    return 0;
}
