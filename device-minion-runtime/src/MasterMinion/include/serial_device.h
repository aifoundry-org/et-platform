// Device-specific serial setings needed by the serial driver

#ifndef SERIAL_DEVICE_H
#define SERIAL_DEVICE_H

#include "UART.h"

#define PU_UART_BASE_ADDRESS 0x0012002000ULL
#define UART0 ((volatile SPIO_UART_t*)PU_UART_BASE_ADDRESS)

#endif
