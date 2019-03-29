#include "master.h"
#include "shire.h"
#include "worker.h"

// Select PU peripherals for initial master minion use
#define PU_PLIC_BASE_ADDRESS  0x0010000000ULL
#define PU_TIMER_BASE_ADDRESS 0x0012005000ULL

int main(void)
{
    // Setup trap handler
    asm volatile ("la t0, trap_handler\n"
	              "csrw mtvec, t0" : : : "t0");

    // enable shadow registers for hartid and sleep txfma
    asm volatile ("csrwi menable_shadows, 0x3");

    if (get_shire_id() == 32)
    {
        MASTER_thread();
    }
    else
    {
        WORKER_thread();
    }

    return 0;
}
