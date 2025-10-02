#include "device-mrt-trace.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

int64_t entry_point(void);

int64_t entry_point(void)
{
    const char alphabet = 'A';
    const size_t string_event_header_size = offsetof(struct trace_string_t, msg);
    const size_t max_str_events_per_buff = 30;
    const size_t extra_events = 4;

    // Trace message size
    size_t message_size = 128;

    // trace message
    char message[message_size];

    /* Case No. 1: Log just a single trace message (string event). */

    // Prepare the message
    memset(message, alphabet, message_size - 1);
    message[message_size - 1] = 0;

    // Leave bytes equal to string event header size and log the rest of the message.
    TRACE_string(LOG_LEVELS_CRITICAL, message + string_event_header_size);

    /* Case No. 2: Fully fill the buffer with messages (string events). A total of 30 messages of
    128-bytes each are going to completely fill the buffer. A different log level is being
    selected. 29 messages will be logged with log level LOG_LEVELS_ERROR . Note that when we selected
    LOG_LEVELS_ERROR as log level, message with critical log level will be logged too, making a total
    of 30 messages in our case (one message with log level critical will be logged).*/

    uint32_t out_counter = 1;

    for (; out_counter < max_str_events_per_buff; out_counter++)
    {
        memset(message, (char)(alphabet + out_counter), message_size - 1);
        TRACE_string(LOG_LEVELS_ERROR, message + string_event_header_size);
    }

    /* Case No. 3: Wrapp the ring buffer. Messages logged more than 30 will make the ring buffer wrapped.
    We will be logging 4 messages to wrapp the buffer. We will be selecting LOG_LEVELS_WARNING as our log level
    for 4 messages. LOG_LEVELS_WARNING will make LOG_LEVELS_ERROR and LOG_LEVELS_CRITICAL to be logged too. So, a 
    total of 34 messages will be logged when LOG_LEVELS_WARNING is selected, and it will make the bufer wrapped.*/

    for (; out_counter < (max_str_events_per_buff + extra_events); out_counter++)
    {
        memset(message, (char)(alphabet + out_counter), message_size - 1);
        TRACE_string(LOG_LEVELS_WARNING, message + string_event_header_size);
    }

    return 0;
}

