

#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
void* asm_memcpy(void* dest, const void* src, size_t n);
void* asm_memset(void* s, int c, size_t n);
#ifdef __cplusplus
}
#endif
