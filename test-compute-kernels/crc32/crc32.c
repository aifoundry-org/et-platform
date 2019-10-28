#include "kernel_params.h"
#include "crc32.h"

#include <stdint.h>
#include <stddef.h>

// tensor_a = source address
// tensor_b = length
// tensor_c = crc32 address
int64_t main(const kernel_params_t* const kernel_params_ptr)
{
    uint32_t crc = 0;

    if ((kernel_params_ptr == NULL) ||
        ((uint64_t*)kernel_params_ptr->tensor_a == NULL) ||
        (kernel_params_ptr->tensor_b == 0) ||
        ((uint32_t*)kernel_params_ptr->tensor_c == NULL))
    {
        // Bad arguments
        return -1;
    }

    const void* data_ptr = (void*)kernel_params_ptr->tensor_a;
    const uint64_t length = kernel_params_ptr->tensor_b;
    uint32_t* const crc_ptr = (uint32_t*)kernel_params_ptr->tensor_c;

    if ((uint64_t)data_ptr % 4)
    {
        // Insufficiently aligned data_ptr
        return -2;
    }

    crc = crc32_8bytes(data_ptr, length, crc);

    *crc_ptr = crc;
    return 0;
}


