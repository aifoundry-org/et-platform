/*
 * Test: decode_power_status_test
 * Fills a trace with random power status updates.
 * This trace is then read and decoded.
 */

#include <stdlib.h>

#include <et-trace/encode.h>
#include <et-trace/decode.h>
#include <et-trace/layout.h>

#include "common/test_trace.h"
#include "common/test_macros.h"
#include "common/user_args.h"

#define rand_u8()  (rand() % (1 << 8))
#define rand_u16() (rand() % (1 << 16))

static void write_random_power_status(struct trace_control_block_t *cb)
{
    struct trace_event_power_status_t power_data = {
        .throttle_state = rand_u8(),
        .power_state = rand_u8(),
        .current_power = rand_u8(),
        .current_temp = rand_u8(),
        .tgt_freq = rand_u16(),
        .tgt_voltage = rand_u16(),
    };
    Trace_Power_Status(cb, &power_data);
}

static void check_random_power_status(const struct trace_event_power_status_t *pwr)
{
    uint8_t throttle_state = rand_u8();
    uint8_t power_state = rand_u8();
    uint8_t current_power = rand_u8();
    uint8_t current_temp = rand_u8();
    uint16_t tgt_freq = rand_u16();
    uint16_t tgt_voltage = rand_u16();
    CHECK_EQ(pwr->throttle_state, throttle_state);
    CHECK_EQ(pwr->power_state, power_state);
    CHECK_EQ(pwr->current_power, current_power);
    CHECK_EQ(pwr->current_temp, current_temp);
    CHECK_EQ(pwr->tgt_freq, tgt_freq);
    CHECK_EQ(pwr->tgt_voltage, tgt_voltage);
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
            write_random_power_status(&cb);
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

    srand(uargs.seed); /* Reset random seed */

    { /* Decode trace buffer */
        printf("-- decoding trace buffer\n");
        struct trace_power_status_t *entry = NULL;
        uint64_t i = 0;
        while (1) {
            entry = Trace_Decode(buf, entry);
            if (!entry)
                break;
            CHECK_EQ(entry->header.type, TRACE_TYPE_POWER_STATUS);
            check_random_power_status(&entry->power);
            ++i;
        }
        CHECK_EQ(i, n_entries);
    }

    test_trace_destroy(buf);

    printf("%s: test passed\n", argv[0]);
}
