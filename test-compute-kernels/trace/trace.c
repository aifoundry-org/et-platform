#include "device-mrt-trace.h"

#include <stdint.h>
#include <stddef.h>

// Use all trace subsystem provided functions
int64_t main(void)
{
    // Call all trace logging functions with dummy values
    TRACE_kernel_launch(LOG_LEVELS_ERROR, 0, 1, 2, 3, 4, 5, 6, 7, 1);
    TRACE_string(LOG_LEVELS_WARNING, "Trace message from kernel");
    TRACE_perfctr(LOG_LEVELS_INFO, 1, 2);
    TRACE_uber_kernel_marker(LOG_LEVELS_DEBUG, 1);

    // Logging kernel_state events with log level number in kernel_state param for log level testing
    TRACE_kernel_state(LOG_LEVELS_CRITICAL, 1, LOG_LEVELS_CRITICAL);
    TRACE_kernel_state(LOG_LEVELS_ERROR, 1, LOG_LEVELS_ERROR);
    return 0;
}
