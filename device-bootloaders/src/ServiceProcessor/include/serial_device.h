// Device-specific serial setings needed by the serial driver

#ifndef SERIAL_DEVICE_H
#define SERIAL_DEVICE_H

#include "UART.h"

#define SPIO_UART0_BASE_ADDRESS 0x0052022000ULL
#define UART0 ((volatile SPIO_UART_t*)SPIO_UART0_BASE_ADDRESS)

#endif
