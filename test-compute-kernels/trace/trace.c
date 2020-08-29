#include "kernel_params.h"
#include "device-mrt-trace.h"

#include <stdint.h>
#include <stddef.h>

// Use all trace subsystem provided functions
int64_t main(const kernel_params_t* const kernel_params_ptr)
{
    // Call all trace logging functions with dummy values
    TRACE_kernel_launch(LOG_LEVELS_ERROR, kernel_params_ptr->tensor_a, kernel_params_ptr->tensor_b,
                        kernel_params_ptr->tensor_c, kernel_params_ptr->tensor_d, kernel_params_ptr->tensor_e,
                        kernel_params_ptr->tensor_f, kernel_params_ptr->tensor_g, kernel_params_ptr->tensor_h, kernel_params_ptr->kernel_id);
    TRACE_string(LOG_LEVELS_WARNING, "Trace message from kernel");
    TRACE_perfctr(LOG_LEVELS_INFO, 1, 2);
    TRACE_uber_kernel_marker(LOG_LEVELS_DEBUG, 1);

    // Logging kernel_state events with log level number in kernel_state param for log level testing
    TRACE_kernel_state(LOG_LEVELS_CRITICAL, 1, LOG_LEVELS_CRITICAL);
    TRACE_kernel_state(LOG_LEVELS_ERROR, 1, LOG_LEVELS_ERROR);
    return 0;
}
