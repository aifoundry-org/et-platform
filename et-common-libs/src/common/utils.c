#include "etsoc/common/utils.h"
#include "etsoc/isa/syscall.h"

#define inhibit_loop_to_libcall \
    __attribute__ ((__optimize__ ("-fno-tree-loop-distribute-patterns")))

void*
inhibit_loop_to_libcall
et_memset(void *s, int c, size_t n)
{
  unsigned char *p = s;

  while (n-- > 0) {
    *p++ = (char)c;
  }

  return s;
}

void *
inhibit_loop_to_libcall
et_memcpy(void *dest, const void *src, size_t n)
{
  const char *s = src;
  char *d = dest;

  while (n) {
    *d++ = *s++;
    n--;
  }

  return dest;
}

int 
inhibit_loop_to_libcall
et_memcmp(const void *s1, const void *s2, size_t n)
{
  const char* p_s1 = s1;
  const char* p_s2 = s2;
  unsigned char u1;
  unsigned char u2;

  for (; n--; p_s1++, p_s2++) {
    u1 = *p_s1;
    u2 = *p_s2;
    if (u1 != u2)
      return u1 - u2;
  }

  return 0;
}

size_t
inhibit_loop_to_libcall
et_strlen(const char *str)
{
  const char *s = str;

  while (*s)
    s++;

  return (size_t)(s - str);
}

__attribute__((noreturn)) void et_abort(void)
{
  syscall(SYSCALL_RETURN_FROM_KERNEL, 0, KERNEL_RETURN_SELF_ABORT, 0);

  /* Should never get here */
  while (1) {
    __asm__ __volatile__("wfi\n");
  }
}
