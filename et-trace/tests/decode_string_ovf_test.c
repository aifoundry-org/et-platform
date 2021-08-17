/*
 * Test: decode_string_ovf
 * Fill the device trace with n_entries strings.
 * The test string is larger than the allocated buffer (rest should be ignored).
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
    static const char test_str[128] = "00112233445566778899aabbccddeeff"
                                      "00112233445566778899aabbccddeeff"
                                      "00112233445566778899aabbccddeeff"
                                      "00112233445566778899aabbccddeeff";

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
        uint64_t i = 0;
        while (1) {
            entry = Trace_Decode(buf, entry);
            if (!entry)
                break;
            CHECK_EQ(ENTRY_HEADER(entry).type, TRACE_TYPE_STRING);
            CHECK_STRNEQ(entry->string, test_str, 64);
            ++i;
        }
        CHECK_EQ(i, n_entries);
    }

    test_trace_destroy(buf);

    printf("%s: test passed\n", argv[0]);
}
