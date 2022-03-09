#pragma once

#include "trace.h"

/* Helper macros for self-checking */
#define _EXPECT_IMPL(a, op, b, file, line, fmt)                                                        \
    do {                                                                                               \
        if (!((a)op(b))) {                                                                             \
            failf("%s:%d: assertion failed: %s %s %s, where %s=" fmt, file, line, #a, #op, #b, #a, a); \
        }                                                                                              \
    } while (0)


#define EXPECT(a, op, b)  _EXPECT_IMPL(a, op, b, __FILE__, __LINE__, "%lld")
#define EXPECTU(a, op, b) _EXPECT_IMPL(a, op, b, __FILE__, __LINE__, "%llu")
#define EXPECTX(a, op, b) _EXPECT_IMPL(a, op, b, __FILE__, __LINE__, "0x%llx")


#define CHECK(x)                                                          \
    do {                                                                  \
        if (!(x)) {                                                       \
            failf("%s:%d: assertion failed: %s", __FILE__, __LINE__, #x); \
        }                                                                 \
    } while (0)
