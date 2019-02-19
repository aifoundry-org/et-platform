#include "et_cru.h"
#include "serial.h"

#include <stdio.h>

// Select SPIO peripherals for initial SP use
#define SPIO_NOC_SPIO_REGBUS_BASE_ADDRESS    0x0040100000ULL
#define SPIO_NOC_PU_MAIN_REGBUS_BASE_ADDRESS 0x0040200000ULL
#define SPIO_NOC_PSHIRE_REGBUS_BASE_ADDRESS  0x0040300000ULL
#define SPIO_SRAM_BASE_ADDRESS               0x0040400000ULL
#define SPIO_SRAM_SIZE                       0x100000UL // 1MB
#define SPIO_MAIN_NOC_REGBUS_BASE_ADDRESS    0x0042000000ULL
#define SPIO_PLIC_BASE_ADDRESS               0x0050000000ULL
#define SPIO_UART0_BASE_ADDRESS              0x0052022000ULL

int main(void)
{
    SERIAL_init(UART0);
    printf("alive\r\n");
    while (1) {}
}
