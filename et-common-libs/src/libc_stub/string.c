#include "string.h"

#include "etsoc/common/utils.h"

void* memset(void* s, int c, size_t n) {
  return et_memset(s, c, n);
}

void* memcpy(void* dest, const void* src, size_t n) {
  return et_memcpy(dest, src, n);
}

int memcmp(const void* s1, const void* s2, size_t n) {
  return et_memcmp(s1, s2, n);
}

size_t strlen(const char* str) {
  return et_strlen(str);
}
