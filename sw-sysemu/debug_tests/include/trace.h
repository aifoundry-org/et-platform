#pragma once

#include "common.h"

#ifdef NTRACE

#define putchar(...)
#define puts(...)
#define printf(...)
#define vprintf(...)
#define tracef(...)
#define failf(...) C_TEST_FAIL

#else /* !NTRACE */

/* Standard print/format */
int putchar(int c);
int puts(const char* s);
int printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
int vprintf(const char* fmt, va_list ap);

/* Trace utils */
void tracef(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void failf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

#endif
