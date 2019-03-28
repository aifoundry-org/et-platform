// printx.h

#ifndef __PRINTX_H__
#define __PRINTX_H__

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

int printx(const char *format, ...);
int snprintx(char *str, size_t size, const char *format, ...);

//int vprintx(const char *format, va_list ap);
//int vsnprintx(char *str, size_t size, const char *format, va_list ap);

#endif
