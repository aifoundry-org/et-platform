#include <stdlib.h>

#include "PLIC.h"

#include "cacheops.h"
#include "macros.h"
#include "shire.h"
#include "fw_master_code.h"
#include "fw_compute_code.h"

#define DRAM_SPACE_BASE_ADDRESS 0x8100000000ULL
#define DRAM_SPACE_SIZE 0x7EFFFFFFFFULL

// Select PU peripherals for initial master minion use
#define PU_PLIC_BASE_ADDRESS  0x10000000ULL
#define PU_UART_BASE_ADDRESS  0x14002000ULL
#define PU_TIMER_BASE_ADDRESS 0x14005000ULL
#define PU_SRAM_BASE_ADDRESS  0x14008000ULL
#define PU_SRAM_SIZE          0x40000UL // 256KB
#define PU_PLL_BASE_ADDRESS   0x1A000000ULL

// Select SPIO peripherals for initial SP use
#define SPIO_NOC_SPIO_REGBUS_BASE_ADDRESS    0x40100000ULL
#define SPIO_NOC_PU_MAIN_REGBUS_BASE_ADDRESS 0x40200000ULL
#define SPIO_NOC_PSHIRE_REGBUS_BASE_ADDRESS  0x40300000ULL
#define SPIO_SRAM_BASE_ADDRESS               0x40400000ULL
#define SPIO_SRAM_SIZE                       0x100000UL // 1MB
#define SPIO_MAIN_NOC_REGBUS_BASE_ADDRESS    0x42000000ULL
#define SPIO_PLIC_BASE_ADDRESS               0x50000000ULL
#define SPIO_UART0_BASE_ADDRESS              0x54002000ULL

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
    if(minion_id >= 1024)
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
