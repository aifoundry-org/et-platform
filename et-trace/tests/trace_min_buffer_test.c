/*
 * Test: trace_min_buffer_test
 * Init Trace with invalid buffer size and try to fill the trace with n_entries strings.
 * This trace is then read and decoded.
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
    static const size_t trace_size = 40;
    static const uint64_t n_entries = 10;
    static const char test_str[64] = "Hello world";

    struct user_args uargs;
    parse_args(argc, argv, &uargs);
    srand(uargs.seed);

    struct trace_control_block_t cb = { 0 };
    struct trace_buffer_std_header_t *buf = malloc(trace_size);
    if (!buf)
    {
        return -1;
    }

    struct trace_init_info_t info;
    info.shire_mask = 0xFFFFFFFF;
    info.thread_mask = 0XFFFFFFFF;
    info.event_mask = TRACE_EVENT_ENABLE_ALL;
    info.filter_mask = TRACE_FILTER_ENABLE_ALL;
    info.threshold = (uint32_t)trace_size;

    cb.size_per_hart = (uint32_t)trace_size;
    cb.base_per_hart = (uint64_t)buf;
    cb.offset_per_hart = sizeof(struct trace_buffer_std_header_t);
    printf("-- initialize trace with invalid buffer size\n");
    int32_t status = Trace_Init(&info, &cb, TRACE_STD_HEADER);
    CHECK_EQ(status, TRACE_INVALID_BUF_SIZE);
    CHECK_EQ(cb.enable, TRACE_DISABLE);

    buf->magic_header = TRACE_MAGIC_HEADER;
    buf->data_size = sizeof(struct trace_buffer_std_header_t);
#ifdef MASTER_MINION
    buf->type = TRACE_MM_BUFFER;
#else
    buf->type = TRACE_CM_BUFFER;
#endif

    printf("-- try to log trace events trace buffer\n");
    /* try to Populate trace buffer, it should not log any data
       into buffer because buffer size is invalid */
    for (uint64_t i = 0; i < n_entries; ++i) {
        Trace_String(TRACE_EVENT_STRING_INFO, &cb, test_str);
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

    /* Decoder trace buffer */
    printf("-- decoding trace buffer\n");
    const struct trace_entry_header_t *entry = NULL;
    entry = Trace_Decode(buf, entry);
    /* Expect that no event was logged into Trace buffer. */
    CHECK_EQ((uint64_t)entry, 0);

    test_trace_destroy(buf);

    printf("%s: test passed\n", argv[0]);
}
