// Device-specific serial setings needed by the serial driver

#ifndef SERIAL_DEVICE_H
#define SERIAL_DEVICE_H

#include "DW_apb_uart.h"

#define UART0_BASE_ADDRESS 0x0012002000ULL
#define UART1_BASE_ADDRESS 0x0012007000ULL

#define UART0 ((volatile Uart_t*)UART0_BASE_ADDRESS)
#define UART1 ((volatile Uart_t*)UART1_BASE_ADDRESS)
#endif
