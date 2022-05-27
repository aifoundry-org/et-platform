#include <stdint.h>
#include "etsoc/isa/cacheops-umode.h"

/* Generates tensor error */
int main(void)
{
    asm volatile("csrw tensor_error, %0\n" : : "I"(0x1));
    return 0;
}
