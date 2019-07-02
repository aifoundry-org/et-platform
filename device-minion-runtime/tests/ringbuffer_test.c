#include "ringbuffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

// gcc -Wall -Wextra -O3 ringbuffer_test.c ../src/shared/src/ringbuffer.c -I ../src/shared/include/
// ./a.out
// checking state after init...ok
// checking state after writing to full...ok
// checking state after reading to empty...ok
// Running random reads and writes...ok

#define MAX_RANDOM_READ_WRITE_LENGTH RINGBUFFER_MAX_LENGTH

static ringbuffer_t ringbuffer;

static int64_t check_state(const ringbuffer_t* const ringbuffer_ptr, uint64_t used, uint64_t length);
static int64_t random_reads_and_writes(ringbuffer_t* const ringbuffer_ptr, uint64_t ringbuffer_length, uint32_t writes);

int main(void)
{
    const uint64_t ringbuffer_length = RINGBUFFER_LENGTH - 1;
    ringbuffer_t* ringbuffer_ptr = &ringbuffer;

    RINGBUFFER_init(ringbuffer_ptr);

    printf("checking state after init...");

    if (check_state(ringbuffer_ptr, 0, ringbuffer_length) < 0)
    {
        return -1;
    }

    printf("ok\r\n");

    // Write to full with a predictable pattern that's unique per entry
    for (uint32_t i = 0; i < ringbuffer_length; i++)
    {
        uint8_t write_data = i & 0xFFU;

        int64_t rv = RINGBUFFER_write(ringbuffer_ptr,  &write_data, 1);

        if (rv != 1)
        {
            printf("write error i = %" PRId32 " got %" PRId64 "\r\n", i, rv);
            return -1;
        }
    }

    printf("checking state after writing to full...");

    if (check_state(ringbuffer_ptr, ringbuffer_length, ringbuffer_length))
    {
        return -1;
    }

    printf("ok\r\n");

    // Read and verify pattern to empty
    for (uint32_t i = 0; i < ringbuffer_length; i++)
    {
        uint8_t read_data;

        if (1 == RINGBUFFER_read(ringbuffer_ptr, &read_data, 1))
        {
            if (read_data != (i & 0xFFU))
            {
                printf("bad read data expected %" PRId32 " read %" PRId32 "\r\n", i, read_data);
                return -1;
            }
        }
    }

    printf("checking state after reading to empty...");

    if (check_state(ringbuffer_ptr, 0, ringbuffer_length))
    {
        return -1;
    }

    printf("ok\r\n");

    printf("Running random reads and writes...");
    fflush(stdout);

    if (random_reads_and_writes(ringbuffer_ptr, ringbuffer_length, 1000000000) < 0)
    {
        return -1;
    }

    printf("ok\r\n");
    return 0;
}

static int64_t check_state(const ringbuffer_t* const ringbuffer_ptr, uint64_t used, uint64_t length)
{
    uint64_t result;
    const bool empty = (used == 0);
    const bool full = (used == length);

    if (RINGBUFFER_empty(ringbuffer_ptr) != empty)
    {
        printf("empty error\r\n");
        return -1;
    }

    if (RINGBUFFER_full(ringbuffer_ptr) != full)
    {
        printf("full error\r\n");
        return -1;
    }

    uint64_t expected = length - used;
    result = RINGBUFFER_free(ringbuffer_ptr);

    if (result != expected)
    {
        printf("free error, expected %" PRIu64 " got %" PRIu64 "\r\n", expected, result);
        return -1;
    }

    result = RINGBUFFER_used(ringbuffer_ptr);

    if (result != used)
    {
        printf("used error, expected %" PRIu64 " got %" PRIu64 "\r\n", used, result);
        return -1;
    }

    return 0;
}

static int64_t random_reads_and_writes(ringbuffer_t* ringbuffer_ptr, uint64_t ringbuffer_length, uint32_t writes)
{
    uint8_t buffer[MAX_RANDOM_READ_WRITE_LENGTH];
    uint64_t free = ringbuffer_length;
    uint64_t used = 0;
    uint32_t write_value = 0;
    uint32_t read_value = 0;

    while (write_value < writes)
    {
        const uint64_t read_length = rand() % MAX_RANDOM_READ_WRITE_LENGTH;
        const uint64_t write_length = rand() % MAX_RANDOM_READ_WRITE_LENGTH;

        //printf("writing %" PRId64 "\r\n", write_length);

        if (write_length <= free)
        {
            // Nth byte written value = N
            for (uint64_t i = 0; i < write_length; i++)
            {
                buffer[i] = (write_value++) & 0xFFU;
            }

            int64_t rv = RINGBUFFER_write(ringbuffer_ptr,  buffer, write_length);

            if (rv < 0)
            {
                printf("random write bad return value, expected >= 0 got %" PRId64 "\r\n", rv);
                return -1;
            }

            used += write_length;
            free -= write_length;
        }
        else
        {
            for (uint64_t i = 0; i < write_length; i++)
            {
                buffer[i] = 0;
            }

            int64_t rv = RINGBUFFER_write(ringbuffer_ptr, buffer, write_length);

            if (rv >= 0)
            {
                printf("random write bad return value, expected < 0 got %" PRId64 "\r\n", rv);
                return -1;
            }
        }

        if (check_state(ringbuffer_ptr, used, ringbuffer_length) < 0)
        {
            return -1;
        }

        //printf("reading %" PRId64 "\r\n", read_length);

        if (read_length <= used)
        {
            int64_t rv = RINGBUFFER_read(ringbuffer_ptr, buffer, read_length);

            if (rv < 0)
            {
                printf("random read bad return value, expected >= 0 got %" PRId64 "\r\n", rv);
                return -1;
            }

            for (uint64_t i = 0; i < read_length; i++)
            {
                const uint8_t expected = (read_value++) & 0xFFU;

                // Verify the value of the Nth byte read = N
                if (buffer[i] != expected)
                {
                    printf("random read data error expected %" PRId32 " got %" PRId32 "\r\n", expected, buffer[i]);
                    return -1;
                }
            }

            used -= read_length;
            free += read_length;
        }
        else
        {
            int64_t rv = RINGBUFFER_read(ringbuffer_ptr, buffer, read_length);

            if (rv >= 0)
            {
                printf("random read bad return value, expected < 0 got %" PRId64 "\r\n", rv);
                return -1;
            }
        }

        if (check_state(ringbuffer_ptr, used, ringbuffer_length) < 0)
        {
            return -1;
        }
    }

    return 0;
}
