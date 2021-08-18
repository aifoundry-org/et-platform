/*
 * Test: decoder_cmd_status_test
 * Fills a trace with cmd status updates.
 * This trace is then read and decoderd.
 */

#include <stdlib.h>

#include <et-trace/encoder.h>
#include <et-trace/decoder.h>
#include <et-trace/layout.h>

#include "common/test_trace.h"
#include "common/test_macros.h"
#include "common/user_args.h"

static void trace_log_cmd_status(struct trace_control_block_t *cb, uint16_t message_id,
                                 uint8_t sqw_idx, uint16_t tag_id, uint8_t status)
{
    struct trace_event_cmd_status_t cmd_data = {
        .queue_slot_id = sqw_idx,
        .mesg_id = message_id,
        .trans_id = tag_id,
        .cmd_status = status,
    };
    Trace_Cmd_Status(cb, &cmd_data);
}

int main(int argc, const char **argv)
{
    static const size_t trace_size = 4096;
    static const uint64_t n_entries = 10;

    struct user_args uargs;
    parse_args(argc, argv, &uargs);

    srand(uargs.seed);

    struct trace_control_block_t cb = { 0 };
    struct trace_buffer_std_header_t *buf = test_trace_create(&cb, trace_size);

    printf("-- populating trace buffer\n");
    { /* Populate trace buffer */
        for (uint64_t i = 0; i < n_entries; ++i) {
            trace_log_cmd_status(&cb, i, i + 1, i + 2, i + 3);
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
        struct trace_cmd_status_t *entry = NULL;
        uint64_t i = 0;
        while (1) {
            entry = Trace_Decode(buf, entry);
            if (!entry)
                break;
            CHECK_EQ(entry->header.type, TRACE_TYPE_CMD_STATUS);
            CHECK_EQ(entry->cmd.mesg_id, i);
            CHECK_EQ(entry->cmd.queue_slot_id, i + 1);
            CHECK_EQ(entry->cmd.trans_id, i + 2);
            CHECK_EQ(entry->cmd.cmd_status, i + 3);
            ++i;
        }
        CHECK_EQ(i, n_entries);
    }

    test_trace_destroy(buf);

    printf("%s: test passed\n", argv[0]);
}
