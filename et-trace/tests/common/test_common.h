#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* clang-format off */
#define CHECK_PRINTF(str, a)                             \
    _Generic(a,                                          \
             int8_t:   fprintf(str, "%d",  (int8_t)a),   \
             int16_t:  fprintf(str, "%d",  (int16_t)a),  \
             int32_t:  fprintf(str, "%d",  (int32_t)a),  \
             int64_t:  fprintf(str, "%ld", (int64_t)a),  \
             uint8_t:  fprintf(str, "%u",  (uint8_t)a),  \
             uint16_t: fprintf(str, "%u",  (uint16_t)a), \
             uint32_t: fprintf(str, "%u",  (uint32_t)a), \
             uint64_t: fprintf(str, "%lu", (uint64_t)a))
/* clang-format on */

#define CHECK_IMPL(file, line, a, b, cmp, req)                \
    do {                                                      \
        if (!((a)cmp(b))) {                                   \
            fprintf(stderr, "error: %s:%d\n", file, line);    \
            fprintf(stderr, " | assertion failed:\n");        \
            fprintf(stderr, " |   " #a " " #cmp " " #b "\n"); \
            fprintf(stderr, " | with expansion:\n");          \
            fprintf(stderr, " |   ");                         \
            CHECK_PRINTF(stderr, a);                          \
            fprintf(stderr, " " #cmp " ");                    \
            CHECK_PRINTF(stderr, b);                          \
            fprintf(stderr, "\n");                            \
            if (req)                                          \
                exit(EXIT_FAILURE);                           \
        }                                                     \
    } while (0)

#define CHECK_STREQ_IMPL(file, line, a, b, req)               \
    do {                                                      \
        if (strcmp(a, b) != 0) {                              \
            fprintf(stderr, "error: %s:%d\n", file, line);    \
            fprintf(stderr, " | assertion failed:\n");        \
            fprintf(stderr, " |   " #a " == " #b "\n");       \
            fprintf(stderr, " | with expansion:\n");          \
            fprintf(stderr, " |   \"%s\" == \"%s\"\n", a, b); \
            if (req)                                          \
                exit(EXIT_FAILURE);                           \
        }                                                     \
    } while (0)

#define CHECK_EQ(a, b) CHECK_IMPL(__FILE__, __LINE__, a, b, ==, 0)
#define CHECK_NE(a, b) CHECK_IMPL(__FILE__, __LINE__, a, b, !=, 0)
#define CHECK_LT(a, b) CHECK_IMPL(__FILE__, __LINE__, a, b, <, 0)
#define CHECK_GT(a, b) CHECK_IMPL(__FILE__, __LINE__, a, b, >, 0)
#define CHECK_LE(a, b) CHECK_IMPL(__FILE__, __LINE__, a, b, <=, 0)
#define CHECK_GE(a, b) CHECK_IMPL(__FILE__, __LINE__, a, b, >=, 0)

#define REQUIRE_EQ(a, b) CHECK_IMPL(__FILE__, __LINE__, a, b, ==, 1)
#define REQUIRE_NE(a, b) CHECK_IMPL(__FILE__, __LINE__, a, b, !=, 1)
#define REQUIRE_LT(a, b) CHECK_IMPL(__FILE__, __LINE__, a, b, <, 1)
#define REQUIRE_GT(a, b) CHECK_IMPL(__FILE__, __LINE__, a, b, >, 1)
#define REQUIRE_LE(a, b) CHECK_IMPL(__FILE__, __LINE__, a, b, <=, 1)
#define REQUIRE_GE(a, b) CHECK_IMPL(__FILE__, __LINE__, a, b, >=, 1)

#define CHECK_STREQ(a, b)   CHECK_STREQ_IMPL(__FILE__, __LINE__, a, b, 0)
#define REQUIRE_STREQ(a, b) CHECK_STREQ_IMPL(__FILE__, __LINE__, a, b, 1)

#endif
