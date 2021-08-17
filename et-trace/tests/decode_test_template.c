/*
 * Test: TODO Insert test name
 * TODO Test description
 */

#include <stdlib.h>

#include <et-trace/encode.h>
#include <et-trace/decode.h>
#include <et-trace/layout.h>

#include "common/test_trace.h"
#include "common/test_macros.h"
#include "common/user_args.h"
#include "common/mock_etsoc.h"

int main(int argc, const char **argv)
{
    /* TODO Set static variables */
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
            /* TODO Write to trace buffer */
            reg_hpmcounter3 += 1; /* Update cycle time */
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

    { /* Decode trace buffer */
        printf("-- decoding trace buffer\n");
        /*TODO Update entry type */
        struct trace_entry_header_t *entry = NULL;
        int i = 0;
        while (1) {
            entry = Trace_Decode(buf, entry);
            if (!entry)
                break;
            /* TODO Check decoded entry, e.g.: */
            /* CHECK_EQ(entry->cycle, i); */
            ++i;
        }
        CHECK_EQ(i, n_entries);
    }

    test_trace_destroy(buf);

    printf("%s: test passed\n", argv[0]);
}
