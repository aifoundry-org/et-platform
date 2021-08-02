/*
 * Test: create_trace
 * Randomly generates a synthetic device trace.
 * This trace is then read and decoded.
 */

#include <stdlib.h>
#include <getopt.h>

#define DEVICE_TRACE_DECODE_IMPL
#include <device_trace.h>
#include <device_trace_decode.h>
#include <device_trace_types.h>

#include "common/test_trace.h"
#include "common/test_common.h"

struct user_args {
    int seed;
    const char *output;
};

void parse_args(int argc, const char **argv, struct user_args *uargs)
{
    opterr = 0;

    uargs->seed = 1453;
    uargs->output = NULL;

    int ret;
    while ((ret = getopt(argc, (char *const *)argv, ":s:o:h")) != -1) {
        switch (ret) {
        case 's':
            uargs->seed = atoi(optarg);
            break;
        case 'o':
            uargs->output = optarg;
            break;
        case 'h':
            printf("usage: %s [-s seed] [-o output]\n", argv[0]);
            exit(EXIT_SUCCESS);
            break;
        case ':':
            fprintf(stderr, "error: missing value for '%s'\n", argv[optind - 1]);
            exit(EXIT_FAILURE);
            break;
        case '?':
        default:
            fprintf(stderr, "error: unknown option '%s'\n", argv[optind - 1]);
            exit(EXIT_FAILURE);
        }
    }
    if (optind < argc) {
        fprintf(stderr, "error: pending arguments\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, const char **argv)
{
    static const size_t trace_size = 4096;
    static const uint32_t test_tag = 0x5AD;
    static const uint64_t n_entries = 10;

    struct user_args uargs;
    parse_args(argc, argv, &uargs);

    srand(uargs.seed);

    struct trace_control_block_t cb = { 0 };
    struct trace_buffer_std_header_t *buf = test_trace_create(&cb, trace_size);

    printf("-- populating trace buffer\n");
    { /* Populate trace buffer */
        for (uint64_t i = 0; i < n_entries; ++i) {
            Trace_Format_String(TRACE_EVENT_STRING_INFO, &cb, "Hello %ld", i);
        }
    }

    test_trace_evict(buf, &cb);

    if (uargs.output) {
        printf("-- writing to '%s'\n", uargs.output);
        FILE *fp = fopen(uargs.output, "w");
        if (fp) {
            fwrite(buf, trace_size, 1, fp);
            fclose(fp);
        }
    }

    { /* Decode trace buffer */
        printf("-- decoding trace buffer\n");
        struct trace_string_t *entry = NULL;
        char expected_str[TRACE_STRING_MAX_SIZE];
        uint64_t i = 0;
        while (1) {
            sprintf(expected_str, "Hello %ld", i);
            entry = Trace_Decode(buf, entry);
            if (!entry)
                break;
            CHECK_EQ(entry->header.type, TRACE_TYPE_STRING);
            REQUIRE_STREQ(entry->string, expected_str);
            ++i;
        }
        REQUIRE_EQ(i, n_entries);
    }

    test_trace_destroy(buf);

    printf("%s: test passed\n", argv[0]);
}
