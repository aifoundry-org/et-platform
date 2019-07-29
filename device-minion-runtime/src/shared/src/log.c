#include "log.h"
#include "hart.h"
#include "message.h"

#include <stddef.h>

// sends a log message from a worker minion to the master minion for display
int64_t LOG_write(const char* const string_ptr, uint64_t length)
{
    static message_t message;

    if (string_ptr == NULL)
    {
        return -1;
    }

    char* data_ptr = (char*)message.data;

    message.id = MESSAGE_ID_LOG_WRITE;
    data_ptr[0] = (uint8_t)length;

    for (uint64_t i = 1; (i < length) && (i < sizeof(message.data)); i++)
    {
        data_ptr[i] = string_ptr[i-1];
    }

    return message_send_worker(get_shire_id(), get_hart_id(), &message);
}
