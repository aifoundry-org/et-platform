/*
 * Test: decode_float
 * Fills a trace with n_entries floats.
 * This trace is then read and decoded.
 */

#include <stdlib.h>

#define ET_TRACE_DECODE_IMPL
#include <device-trace/et_trace.h>
#include <device-trace/et_trace_decode.h>
#include <device-trace/et_trace_layout.h>

#include "common/test_trace.h"
#include "common/test_macros.h"
#include "common/user_args.h"

int main(int argc, const char **argv)
{
    static const size_t trace_size = 4096;
    static const uint32_t test_tag = 0x5AD;
    static const uint64_t n_entries = 10;
    static const size_t n_counters = 7;

    struct user_args uargs;
    parse_args(argc, argv, &uargs);

    struct trace_control_block_t cb = { 0 };
    struct trace_buffer_std_header_t *buf = test_trace_create(&cb, trace_size);

    srand(uargs.seed);
    {
        printf("-- populating trace buffer\n");
        for (uint64_t i = 0; i < n_entries; ++i) {
            Trace_Value_float(&cb, test_tag, i + 0.5f);
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
        struct trace_value_float_t *entry = NULL;
        uint64_t i = 0;
        while (1) {
            entry = Trace_Decode(buf, entry);
            if (!entry)
                break;
            CHECK_EQ(ENTRY_HEADER(entry).type, TRACE_TYPE_VALUE_FLOAT);
            CHECK_EQ(entry->tag, test_tag);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
            CHECK_EQ(entry->value, i + 0.5f);
#pragma GCC diagnostic pop
            ++i;
        }
        CHECK_EQ(i, n_entries);
    }

    test_trace_destroy(buf);

    printf("%s: test passed\n", argv[0]);
}
