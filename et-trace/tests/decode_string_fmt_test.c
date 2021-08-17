/*
 * Test: decode_string_fmt
 * Fills a trace with n_entries formatted strings.
 * This trace is then read and decoded.
 */

#include <stdlib.h>

#include <et-trace/encode.h>
#include <et-trace/decode.h>
#include <et-trace/layout.h>

#include "common/test_trace.h"
#include "common/test_macros.h"
#include "common/user_args.h"

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
            CHECK_STREQ(entry->string, expected_str);
            ++i;
        }
        CHECK_EQ(i, n_entries);
    }

    test_trace_destroy(buf);

    printf("%s: test passed\n", argv[0]);
}
