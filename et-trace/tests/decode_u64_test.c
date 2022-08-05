/*
 * Test: decoder_u64
 * Fills a trace with n_entries u64 values.
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
    static const size_t trace_size = 4096;
    static const uint32_t test_tag = 0x5AD;
    static const uint64_t n_entries = 10;

    struct user_args uargs;
    parse_args(argc, argv, &uargs);

    struct trace_control_block_t cb = { 0 };
    struct trace_buffer_std_header_t *buf = test_trace_create(&cb, trace_size);

    srand(uargs.seed);
    {
        printf("-- populating trace buffer\n");
        for (uint64_t i = 0; i < n_entries; ++i) {
            Trace_Value_u64(&cb, test_tag, i);
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

    srand(uargs.seed);
    {
        printf("-- decoding trace buffer\n");
        const struct trace_entry_header_t *entry_header = NULL;
        const struct trace_value_u64_t *entry = NULL;
        uint64_t i = 0;
        while (1) {
            entry_header = Trace_Decode(buf, entry_header);
            if (!entry_header)
                break;
            CHECK_EQ(entry_header->type, TRACE_TYPE_VALUE_U64);
            entry = (const struct trace_value_u64_t *)entry_header;
            CHECK_EQ(entry->tag, test_tag);
            CHECK_EQ(entry->value, i);
            ++i;
        }
        CHECK_EQ(i, n_entries);
    }

    test_trace_destroy(buf);

    printf("%s: test passed\n", argv[0]);
}
