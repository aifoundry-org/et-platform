#include <stdint.h>

#include "constant_memory_compare.h"

int constant_time_memory_compare(volatile const void *restrict s1, volatile const void *restrict s2,
                                 size_t size)
{
    volatile const uint8_t *p1 = (volatile const uint8_t *)s1;
    volatile const uint8_t *p2 = (volatile const uint8_t *)s2;
    volatile const uint8_t *p1_end = p1 + size;
    uint8_t c = 0;
    uint8_t t;

    while (p1 < p1_end) {
        t = (*p1) ^ (*p2);
        c = c | t;
        p1++;
        p2++;
    }

    return ((0 == c) || (0 == size)) ? 0 : 1;
}
