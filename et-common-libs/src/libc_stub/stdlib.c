#include "stdlib.h"

#include "etsoc/common/utils.h"

void abort(void) {
  et_abort();
}

/*
 * default assert handler to cope with direct calls into assert() not being intercepted
 * to et_assert() */
void __assert_func(const char *, int, const char *, const char *) __attribute__((__noreturn__));
void __assert_func(const char *file, int line, const char *function, const char *expr)
{
    et_printf("Assertion %s failed: file %s line %d \n %s", expr, file, line, function);
    et_abort();
}
