#include <stdint.h>
#include "etsoc/isa/cacheops-umode.h"

int entry_point(void);

/* Generates tensor error */
int entry_point(void)
{
    asm volatile("csrw tensor_error, %0\n" : : "I"(0x1));
    return 0;
}
