/*
 * Test: trace_config_test
 * Fills a trace with n_entries strings.
 * Then disable all events. And try to add more entries.
 * This trace then decode and expects only n entries in buffer.
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
    static const uint64_t n_entries = 10;
    static const char test_str[64] = "Hello world";

    struct user_args uargs;
    parse_args(argc, argv, &uargs);

    srand(uargs.seed);

    struct trace_control_block_t cb = { 0 };
    struct trace_buffer_std_header_t *buf = test_trace_create(&cb, trace_size);

    printf("-- populating trace buffer\n");
    { /* Populate trace buffer */
        for (uint64_t i = 0; i < n_entries; ++i) {
            Trace_String(TRACE_EVENT_STRING_INFO, &cb, test_str);
        }
    }

    struct trace_config_info_t trace_config = { .filter_mask = 0,
        .event_mask = 0,
        .threshold = (uint32_t)(trace_size / 2)};

    printf("-- Configuring trace to disable all events.\n");
    int32_t status = Trace_Config(&trace_config, &cb);
    CHECK_EQ(status, TRACE_STATUS_SUCCESS);
    CHECK_EQ(cb.event_mask, trace_config.event_mask);
    CHECK_EQ(cb.filter_mask, trace_config.filter_mask);
    CHECK_EQ(cb.threshold, trace_config.threshold);

    printf("-- try adding more packets in trace buffer\n");
    { /* When all events disabled, these should not be logged into trace */
        for (uint64_t i = 0; i < n_entries; ++i) {
            Trace_String(TRACE_EVENT_STRING_INFO, &cb, test_str);
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

    { /* Decoder trace buffer */
        printf("-- decoding trace buffer\n");
	const struct trace_entry_header_t *entry_header = NULL;
        const struct trace_string_t *entry = NULL;
        uint64_t i = 0;
        while (1) {
            entry_header = Trace_Decode(buf, entry_header);
            if (!entry_header)
                break;
            CHECK_EQ(entry_header->type, TRACE_TYPE_STRING);
            entry = (const struct trace_string_t *)entry_header;
            CHECK_STREQ(entry->string, test_str);
            ++i;
        }
        CHECK_EQ(i, n_entries);
    }

    test_trace_destroy(buf);

    printf("%s: test passed\n", argv[0]);
}
