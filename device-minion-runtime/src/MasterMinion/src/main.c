#include <stdlib.h>

#include "PLIC.h"

#include "cacheops.h"
#include "macros.h"
#include "shire.h"
#include "fw_master_code.h"
#include "fw_compute_code.h"
#include "serial.h"

#define DRAM_BASE_ADDRESS 0x8000000000ULL
#define DRAM_SIZE 0x800000000ULL

// Select PU peripherals for initial master minion use
#define PU_PLIC_BASE_ADDRESS  0x0010000000ULL
#define PU_TIMER_BASE_ADDRESS 0x0012005000ULL

int main(void)
{
    __asm__ __volatile__ (
       // enable shadow registers for hartid and sleep txfma
       "csrwi 0x7d2, 0x3\n"
       :
       :
       : "t0"
    );

    // Gets the minion id
    unsigned int minion_id = get_minion_id();

    // Master shire, go to master code
    if(minion_id >= 32)
    {
        fw_master_code();
    }
    // Compute shire, go to compute code
    else
    {
        fw_compute_code();
    }

    return 0;
}
