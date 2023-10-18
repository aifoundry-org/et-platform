
#include "crc32.h"

#include <stdint.h>
#include <stddef.h>

typedef struct {
  void* data_ptr;
  uint64_t length;
  uint32_t* crc_ptr;
}  Parameters;
int64_t entry_point(const Parameters* const kernel_params_ptr)
{
    uint32_t crc = 0;

    if ((kernel_params_ptr == NULL) ||
        kernel_params_ptr->data_ptr == NULL ||
        kernel_params_ptr->length == 0 ||
        kernel_params_ptr->crc_ptr == NULL)
    {
        // Bad arguments
        return -1;
    }

    const void* data_ptr = kernel_params_ptr->data_ptr;
    const uint64_t length = kernel_params_ptr->length;
    uint32_t* const crc_ptr = kernel_params_ptr->crc_ptr;

    if ((uint64_t)data_ptr % 4)
    {
        // Insufficiently aligned data_ptr
        return -2;
    }

    crc = crc32_8bytes(data_ptr, length, crc);

    *crc_ptr = crc;
    return 0;
}


