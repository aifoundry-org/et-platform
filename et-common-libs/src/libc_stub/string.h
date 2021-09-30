#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* memset(void* s, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);
size_t strlen(const char* str);

#ifdef __cplusplus
}
#endif