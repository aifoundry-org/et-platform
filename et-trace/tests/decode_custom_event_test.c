/*
 * Test: decode_custom_event_test
 * Fills a custom trace event with user defined size and payload.
 * This trace is then read and decoded.
 */

#include <stdlib.h>

#include <et-trace/encoder.h>
#include <et-trace/decoder.h>
#include <et-trace/layout.h>

#include "common/test_trace.h"
#include "common/test_macros.h"
#include "common/user_args.h"
#include "common/mock_etsoc.h"

int main(int argc, const char **argv)
{
    static char event_payload[] = { "This is a ET custom event" };
    static size_t trace_size = 4096;
    static int n_entries = 10;

    struct user_args uargs;
    parse_args(argc, argv, &uargs);

    srand(uargs.seed);

    struct trace_control_block_t cb = { 0 };
    struct trace_buffer_std_header_t *buf = test_trace_create(&cb, trace_size);

    printf("-- populating trace buffer\n");
    { /* Populate trace buffer */
        for (int i = 0; i < n_entries; ++i) {
            Trace_Custom_Event(&cb, i, (uint8_t*)event_payload, sizeof(event_payload));
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

    /* Reset globals */
    srand(uargs.seed);
    etsoc_reset();

    { /* Decoder trace buffer */
        printf("-- decoding trace buffer\n");
        const struct trace_custom_event_t *entry = NULL;
        int i = 0;
        while (1) {
            entry = Trace_Decode(buf, entry);
            if (!entry)
                break;
            CHECK_EQ(entry->header.type, TRACE_TYPE_CUSTOM_EVENT);
            CHECK_EQ(entry->custom_type, i);
            CHECK_EQ(entry->payload_size, sizeof(event_payload));
            CHECK_STREQ(entry->payload, event_payload);
            ++i;
        }
        CHECK_EQ(i, n_entries);
    }

    test_trace_destroy(buf);

    printf("%s: test passed\n", argv[0]);
}
