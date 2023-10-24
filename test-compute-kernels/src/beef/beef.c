#include "etsoc/isa/hart.h"

#include <stdint.h>
#include <stddef.h>

typedef struct {
  uint64_t* buffer;
  uint64_t numElements;
} MyVectors;

int64_t entry_point(const MyVectors*);

// Writes BEEF
int64_t entry_point(const MyVectors* const vectors) {
    // Only run on one minion
    if (get_hart_id() != 0) {
        return 0;
    }

    if ((vectors == NULL) ||
        ((uint64_t*)vectors->buffer == NULL) ||
        (vectors->numElements == 0))
    {
        // Bad arguments
        return -1;
    }

    volatile uint64_t* data_ptr = (uint64_t*)vectors->buffer;
    const uint64_t length = vectors->numElements;

    if ((uint64_t)data_ptr % 8)
    {
        // Insufficiently aligned data_ptr
        return -2;
    }

    for (uint64_t i = 0; i < (length / 8); i++)
    {
        *data_ptr++ = 0xBEEFBEEFBEEFBEEFULL;
    }

    data_ptr = (uint64_t*)vectors->buffer;

    // Read back data and return comparison pass/fail
    for (uint64_t i = 0; i < (length / 8); i++)
    {
        if (*data_ptr++ != 0xBEEFBEEFBEEFBEEFULL)
        {
            // Miscompare
            return -3;
        }
    }

    return 0;
}
