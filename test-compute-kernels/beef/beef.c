#include "kernel_params.h"

#include <stdint.h>
#include <stddef.h>

// Writes BEEF
// tensor_a = address
// tensor_b = length
int64_t main(const kernel_params_t* const kernel_params_ptr)
{
    if ((kernel_params_ptr == NULL) ||
        ((uint64_t*)kernel_params_ptr->tensor_a == NULL) ||
        (kernel_params_ptr->tensor_b == 0))
    {
        // Bad arguments
        return -1;
    }

    uint64_t* data_ptr = (uint64_t*)kernel_params_ptr->tensor_a;
    const uint64_t length = kernel_params_ptr->tensor_b;

    if ((uint64_t)data_ptr % 8)
    {
        // Insufficiently aligned data_ptr
        return -2;
    }

    for (uint64_t i = 0; i < (length / 8); i++)
    {
        *data_ptr++ = 0xBEEFBEEFBEEFBEEFULL;
    }

    return 0;
}
