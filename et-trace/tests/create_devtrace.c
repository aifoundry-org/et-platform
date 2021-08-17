/*
 * Test: create devtrace
 * Create a random device trace with the following types of events:
 *  - Command Status Update
 *  - Power Status Update
 *  - Performance Counters
 */

#include <stdlib.h>

#include <device-trace/et_trace.h>
#include <device-trace/et_trace_layout.h>

#include "common/test_trace.h"
#include "common/test_macros.h"
#include "common/user_args.h"
#include "common/mock_etsoc.h"

static int randint(int a, int b)
{
    int d = b - a + 1;
    return a + (rand() % d);
}

static void write_queue(struct trace_control_block_t *cb, struct trace_event_cmd_status_t *queue)
{
    static int trans_id = 0;
    switch (queue->cmd_status) {
    case CMD_STATUS_WAIT_BARRIER:
        queue->cmd_status = CMD_STATUS_RECEIVED;
        break;
    case CMD_STATUS_RECEIVED:
        queue->cmd_status = CMD_STATUS_EXECUTING;
        break;
    case CMD_STATUS_EXECUTING: {
        switch (rand() % 3) {
        case 0:
            queue->cmd_status = CMD_STATUS_FAILED;
            break;
        case 1:
            queue->cmd_status = CMD_STATUS_ABORTED;
            break;
        case 2:
            queue->cmd_status = CMD_STATUS_SUCCEEDED;
            break;
        }
        break;
    }
    default:
        queue->cmd_status = rand() & 1 ? CMD_STATUS_WAIT_BARRIER : CMD_STATUS_RECEIVED;
        queue->mesg_id = randint(512, 536);
        queue->trans_id = ++trans_id;
        break;
    }

    Trace_Cmd_Status(cb, queue);
};

static void write_counter(struct trace_control_block_t *cb)
{
    if (rand() & 1) {
        reg_hpmcounter4 += randint(1000, 2000);
        Trace_PMC_Counter(cb, PMC_COUNTER_HPMCOUNTER4);
    } else {
        reg_hpmcounter5 += randint(1000, 2000);
        Trace_PMC_Counter(cb, PMC_COUNTER_HPMCOUNTER5);
    }
}

static void write_power(struct trace_control_block_t *cb)
{
    struct trace_event_power_status_t power_data = {
        .throttle_state = randint(1, 10),
        .power_state = randint(1, 10),
        .current_power = randint(200, 400),
        .current_temp = randint(80, 120),
        .tgt_freq = randint(1000, 2000),
        .tgt_voltage = randint(1, 5),
    };
    Trace_Power_Status(cb, &power_data);
}

int main(int argc, const char **argv)
{
    /* TODO Set static variables */
    static const size_t trace_size = 4096 * 4;
    static const int n_entries = 500;
    static const int n_queues = 4;

    struct user_args uargs;
    parse_args(argc, argv, &uargs);

    srand(uargs.seed);

    struct trace_control_block_t cb = { 0 };
    struct trace_buffer_std_header_t *buf = test_trace_create(&cb, trace_size);

    struct trace_event_cmd_status_t queues[n_queues] = {};
    for (int i = 0; i < n_queues; ++i) {
        queues[i].queue_slot_id = i;
        queues[i].cmd_status = CMD_STATUS_SUCCEEDED;
    }

    printf("-- populating trace buffer\n");
    { /* Populate trace buffer */
        for (int i = 0; i < n_entries; ++i) {
            reg_hpmcounter3 += randint(1000, 2000); /* Update cycle time */
            switch (i % 3) {
            case 0:
                write_queue(&cb, queues + (rand() % n_queues));
                break;
            case 1:
                write_counter(&cb);
                break;
            case 2:
                write_power(&cb);
                break;
            }
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
}
