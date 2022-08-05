/*
 * Test: decode_execution_stack_test
 * Fills a trace with device execution stack updates.
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

static void trace_log_execution_stack(struct trace_control_block_t *cb, uint64_t iter)
{
    struct dev_context_registers_t regs = {.epc = iter, .tval = iter + 1,
        .status = iter + 2, .cause = iter + 3};
    /* Populate GPRs */
    for (unsigned int i = 0; i < TRACE_DEV_CONTEXT_GPRS; i++)
    {
        regs.gpr[i] = iter + 4 + i;
    }
    /* Log the event */
    Trace_Execution_Stack(cb, &regs);
}

int main(int argc, const char **argv)
{
    static size_t trace_size = 4096;
    static unsigned int n_entries = 10;

    struct user_args uargs;
    parse_args(argc, argv, &uargs);

    srand(uargs.seed);

    struct trace_control_block_t cb = { 0 };
    struct trace_buffer_std_header_t *buf = test_trace_create(&cb, trace_size);

    printf("-- populating trace buffer\n");
    { /* Populate trace buffer */
        for (unsigned int i = 0; i < n_entries; ++i) {
            trace_log_execution_stack(&cb, i);
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
        const struct trace_entry_header_t *entry_header = NULL;
        const struct trace_execution_stack_t *entry = NULL;
        unsigned int i = 0;
        while (1) {
            entry_header = Trace_Decode(buf, entry_header);
            if (!entry_header)
                break;
            CHECK_EQ(entry_header->type, TRACE_TYPE_EXCEPTION);
            entry = (const struct trace_execution_stack_t *)entry_header;
            CHECK_EQ(entry->registers.epc, i);
            CHECK_EQ(entry->registers.tval, i + 1);
            CHECK_EQ(entry->registers.status, i + 2);
            CHECK_EQ(entry->registers.cause, i + 3);
            for (unsigned int j = 0; j < TRACE_DEV_CONTEXT_GPRS; j++)
            {
                CHECK_EQ(entry->registers.gpr[j], (uint64_t)(i + 4U + j));
            }
            ++i;
        }
        CHECK_EQ(i, n_entries);
    }

    test_trace_destroy(buf);

    printf("%s: test passed\n", argv[0]);
}
