#include "hart.h"
#include "lfsr.h"
#include "kernel_params.h"

#include <stddef.h>
#include <stdint.h>

int64_t main(const kernel_params_t* const kernel_params_ptr)
{
    if ((kernel_params_ptr == NULL) ||
        ((uint64_t*)kernel_params_ptr->tensor_a == NULL) ||
        ((kernel_params_ptr->tensor_b % 8) != 0) ||
        (kernel_params_ptr->tensor_c == 0))
    {
        // Bad arguments
        return -1;
    }

    // Only the first HART in each shire does any work.
    // Probably only want to run this on one shire.
    if (get_hart_id() % 64 != 0)
    {
        // Nothing to do
        return 0;
    }

    uint64_t* data_ptr = (void*)kernel_params_ptr->tensor_a;
    uint64_t length = kernel_params_ptr->tensor_b;
    uint64_t lfsr = kernel_params_ptr->tensor_c;

    while (length)
    {
        *data_ptr++ = update_lfsr(lfsr);
        length -= 8;
    }

    return 0;
}
