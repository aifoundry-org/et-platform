/*
 * Test: create_trace
 * Randomly generates a synthetic device trace.
 * This trace is then read and decoded.
 */

#include <stdlib.h>

#define MASTER_MINION
#define DEVICE_TRACE_DECODE_IMPL

#include <device_trace_types.h>
#include <device_trace.h>
#include <device_trace_decode.h>

#include "test_trace.h"
#include "test_common.h"

int main(int argc, const char **argv)
{
    static const size_t trace_size = 4096;
    static const uint32_t test_tag = 0x5AD;
    static const uint64_t n_entries = 0;

    int seed = argc > 1 ? atoi(argv[1]) : 1453;
    srand(seed);

    struct trace_control_block_t cb = { 0 };
    struct trace_buffer_std_header_t *buf = test_trace_create(&cb, trace_size);

    { /* Populate trace buffer */
        for (uint64_t i = 0; i < n_entries; ++i) {
            Trace_String(TRACE_EVENT_STRING_INFO, &cb, "Hello World");
        }
    }

    test_trace_evict(buf, &cb);

    { /* Write to file */
        FILE *fp = fopen("test.trace", "w");
        if (fp) {
            fwrite(buf, trace_size, 1, fp);
            fclose(fp);
        }
    }

    { /* Decode trace buffer */
        struct trace_string_t *entry = NULL;
        uint64_t i = 0;
        while (1) {
            entry = Trace_Decode(buf, entry);
            if (!entry)
                break;
            CHECK_EQ((uint64_t)entry, (uint64_t)buf);
            CHECK_EQ(entry->header.type, TRACE_TYPE_STRING);
            REQUIRE_STREQ(entry->string, "Hello world");
            ++i;
        }
        REQUIRE_EQ(i, n_entries);
    }

    test_trace_destroy(buf);

    printf("%s: Test passed\n", argv[0]);
}
