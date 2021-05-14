#ifndef __CONSTANT_MEMORY_COMPARE_H__
#define __CONSTANT_MEMORY_COMPARE_H__

#include <stdlib.h>

int constant_time_memory_compare(volatile const void *restrict s1, volatile const void *restrict s2,
                                 size_t size);

#endif
