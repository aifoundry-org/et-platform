/*
 * Test: decoder_string_test
 * Fills a trace with n_entries strings.
 * This trace is then read and decoderd.
 */

#include <stdlib.h>

#include <et-trace/encoder.h>
#include <et-trace/decoder.h>
#include <et-trace/layout.h>

#include "common/test_trace.h"
#include "common/test_macros.h"
#include "common/user_args.h"

int main(int argc, const char **argv)
{
    static const size_t sub_buffer_count = 8;
    static const size_t trace_size = 4096;
    static const uint64_t n_entries = 10;
    static const char test_str[64] = "Hello world";

    struct user_args uargs;
    parse_args(argc, argv, &uargs);

    srand(uargs.seed);

    struct trace_control_block_t cb[sub_buffer_count];
    struct trace_buffer_std_header_t *buf = test_cm_trace_create(cb, trace_size, sub_buffer_count);

    printf("-- populating trace buffer\n");
    for(uint32_t cbIdx = 0; cbIdx < sub_buffer_count; cbIdx++) {
        for (uint64_t i = 0; i < n_entries; ++i) {
            Trace_String(TRACE_EVENT_STRING_INFO, &cb[cbIdx], test_str);
        }
        test_cm_trace_evict(&cb[cbIdx]);
    }

    if (uargs.output) {
        printf("-- writing to '%s'\n", uargs.output);
        FILE *fp = fopen(uargs.output, "w");
        if (fp) {
            fwrite(buf, trace_size, 1, fp);
            fclose(fp);
        }
    }

    { /* Decoder trace buffer */
        printf("-- decoding trace buffer\n");
        const struct trace_string_t *entry = NULL;
        uint64_t i = 0;
        while (1) {
            entry = Trace_Decode(buf, entry);
            if (!entry)
                break;
            CHECK_EQ(entry->header.type, TRACE_TYPE_STRING);
            CHECK_STREQ(entry->string, test_str);
            ++i;
        }
        CHECK_EQ(i, sub_buffer_count * n_entries);
    }

    test_trace_destroy(buf);

    printf("%s: test passed\n", argv[0]);
}
