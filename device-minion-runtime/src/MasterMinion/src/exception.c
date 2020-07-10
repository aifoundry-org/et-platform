#include "log.h"
#include "macros.h"

void exception_handler(void);

void exception_handler(void)
{
    // TODO exception decoding
    uint64_t scause;
    asm volatile("csrr %0, scause\n" : "=r"(scause));

    log_write(LOG_LEVEL_CRITICAL, "MasterMinon exception: unhandled exception: %lx\n", scause);
    C_TEST_FAIL
}
