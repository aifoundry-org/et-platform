/*-------------------------------------------------------------------------
* Copyright (C) 2019, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#ifndef _COMMON_UTILS_H_
#define _COMMON_UTILS_H_

#include <stddef.h>
#include "trace/trace_umode.h"

/*! \def et_printf(fmt)
    \brief Write a log with va_list style args
    \param fmt String format specifier
    \param ... Variable argument list
*/
#define et_printf(fmt, ...)      __et_printf(fmt, ##__VA_ARGS__)

/*! \fn void *et_memset(void *s, int c, size_t n)
    \brief Copies the character c to the first n characters of the string pointed argument s.
    \param s Pointer to memory control block
    \param c The value to be set
    \param n Number of bytes
    \return Pointer to memory area s
*/
void *et_memset(void *s, int c, size_t n);

/*! \fn void *et_memcpy(void *dest, const void *src, size_t n)
    \brief Copies n characters from memory area src to memory area dest.
    \param dest This is pointer to the destination buffer
    \param src This is pointer to the source buffer
    \param n Number of bytes
    \return Pointer to destination buffer
*/
void *et_memcpy(void *dest, const void *src, size_t n);

/*! \fn int et_memcmp(const void *s1, const void *s2, size_t n)
    \brief Compares the first n bytes of memory area s1 and memory area s2.
    \param s1 This is pointer to a buffer
    \param s2 This is pointer to a buffer
    \param n Number of bytes to compare
    \return < 0 then it indicates s1 is less than s2, > 0 then it indicates s2
    is less than s1, = 0 then it indicates s1 is equal to s2
*/
int et_memcmp(const void *s1, const void *s2, size_t n);

/*! \fn size_t et_strlen(const char *str)
    \brief Computes the length of the string str up to, but not including the
    terminating null character.
    \param str String whos length needs to be found
    \return Length of string
*/
size_t et_strlen(const char *str);

/*! \fn void et_abort(void)
    \brief Causes an abnormal end of the application and returns from kernel launch.
    \return None
*/
__attribute__((noreturn)) void et_abort(void);

/*! \def et_assert(expr)
    \brief A macro that allows diagnostic information to be written to the trace buffer
    and abort the appilcation if the expr provided is false.
*/
#define et_assert(expr)                                                 \
    if (!(expr))                                                        \
    {                                                                   \
        /* TODO: Log the failure to trace buffer (if available)         \
        printf("Assertion \"%s\" failed: file \"%s\", line %d%s%s\n",   \
                expr, __FILE__, __LINE__,                               \
                __FUNCTION__ ? ", function: " : "",                     \
                __FUNCTION__ ? __FUNCTION__ : ""); */                   \
        et_abort();                                                     \
    }                                                                   \
    else                                                                \
    {                                                                   \
        (void)(expr);                                                   \
    }

#endif /* _COMMON_UTILS_H_ */
