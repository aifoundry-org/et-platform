#include "log.h"
#include "macros.h"

void exception_handler(void);

void exception_handler(void)
{
    /* Should never get here, ecalls are handled elsewhere and no other exceptions are delegated to supervisor */
    uint64_t scause;
    asm volatile("csrr %0, scause\n" : "=r"(scause));

    log_write(LOG_LEVEL_CRITICAL, "WorkerMinon exception: unhandled exception: %lx\n", scause);
    C_TEST_FAIL
}
