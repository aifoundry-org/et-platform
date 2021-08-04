/*
 * Test: decode_mixed_test
 * Creates a random trace of different sized integers.
 * This trace is then read and decoded.
 */

#include <stdlib.h>
#include <getopt.h>

#define DEVICE_TRACE_DECODE_IMPL
#include <device_trace.h>
#include <device_trace_decode.h>
#include <device_trace_types.h>

#include "common/test_trace.h"
#include "common/test_macros.h"
#include "common/mock_etsoc.h"

struct user_args {
    int seed;
    const char *output;
};

static void parse_args(int argc, const char **argv, struct user_args *uargs)
{
    opterr = 0;

    uargs->seed = 1453;
    uargs->output = NULL;

    int ret;
    while ((ret = getopt(argc, (char *const *)argv, ":s:o:h")) != -1) {
        switch (ret) {
        case 's':
            uargs->seed = atoi(optarg);
            break;
        case 'o':
            uargs->output = optarg;
            break;
        case 'h':
            printf("usage: %s [-s seed] [-o output]\n", argv[0]);
            exit(EXIT_SUCCESS);
            break;
        case ':':
            fprintf(stderr, "error: missing value for '%s'\n", argv[optind - 1]);
            exit(EXIT_FAILURE);
            break;
        case '?':
        default:
            fprintf(stderr, "error: unknown option '%s'\n", argv[optind - 1]);
            exit(EXIT_FAILURE);
        }
    }
    if (optind < argc) {
        fprintf(stderr, "error: pending arguments\n");
        exit(EXIT_FAILURE);
    }
}

static void write_random_entry(struct trace_control_block_t *cb)
{
    int type = rand() % 4;
    reg_mhartid = rand() % 16;
    reg_hpmcounter3 += 1;
    uint16_t tag = rand() & 0xffff;
    uint8_t value = rand() & 0xff;
    switch (type) {
    case 0:
        Trace_Value_u8(cb, tag, value);
        break;
    case 1:
        Trace_Value_u16(cb, tag, value);
        break;
    case 2:
        Trace_Value_u32(cb, tag, value);
        break;
    case 3:
        Trace_Value_u64(cb, tag, value);
        break;
    }
}

#ifdef MASTER_MINION
#define CHECK_HARTID(entry, hartid) CHECK_EQ(entry->header.hart_id, hartid)
#else
#define CHECK_HARTID(entry, hartid) (void)hartid
#endif

#define CHECK_ENTRY(shtype, type)                                                  \
    static void check_entry_##shtype(const struct trace_value_##shtype##_t *entry) \
    {                                                                              \
        uint64_t mhartid = rand() % 16;                                            \
        reg_hpmcounter3 += 1;                                                      \
        uint16_t tag = rand() & 0xffff;                                            \
        type value = rand() & 0xff;                                                \
        CHECK_HARTID(entry, mhartid);                                              \
        CHECK_EQ(entry->header.cycle, reg_hpmcounter3);                            \
        CHECK_EQ(entry->tag, tag);                                                 \
        CHECK_EQ(entry->value, value);                                             \
    }

CHECK_ENTRY(u8, uint8_t)
CHECK_ENTRY(u16, uint16_t)
CHECK_ENTRY(u32, uint32_t)
CHECK_ENTRY(u64, uint64_t)

static void check_random_entry(const struct trace_entry_header_t *entry)
{
    switch (rand() % 4) {
    case 0:
        check_entry_u8(entry);
        break;
    case 1:
        check_entry_u16(entry);
        break;
    case 2:
        check_entry_u32(entry);
        break;
    case 3:
        check_entry_u64(entry);
        break;
    }
}

int main(int argc, const char **argv)
{
    static const size_t trace_size = 4096 * 4;
    static const uint32_t test_tag = 0x5AD;
    static const int n_entries = 10;

    struct user_args uargs;
    parse_args(argc, argv, &uargs);

    srand(uargs.seed);

    struct trace_control_block_t cb = { 0 };
    struct trace_buffer_std_header_t *buf = test_trace_create(&cb, trace_size);

    printf("-- populating trace buffer\n");
    { /* Populate trace buffer */
        for (int i = 0; i < n_entries; ++i) {
            write_random_entry(&cb);
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

    srand(uargs.seed);
    etsoc_reset();

    { /* Decode trace buffer */
        printf("-- decoding trace buffer\n");
        struct trace_string_t *entry = NULL;
        uint64_t i = 0;
        while (1) {
            printf("-- entry #%ld\n", i);
            entry = Trace_Decode(buf, entry);
            if (!entry)
                break;
            check_random_entry(entry);
            ++i;
        }
        CHECK_EQ(i, n_entries);
    }

    test_trace_destroy(buf);

    printf("%s: test passed\n", argv[0]);
}
