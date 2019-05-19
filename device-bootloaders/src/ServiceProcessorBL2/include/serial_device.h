// Device-specific serial setings needed by the serial driver

#ifndef SERIAL_DEVICE_H
#define SERIAL_DEVICE_H

#include "DW_apb_uart.h"

#define SPIO_UART0_BASE_ADDRESS 0x0052022000ULL
#define SPIO_UART1_BASE_ADDRESS 0x0054052000ULL
#define PU_UART0_BASE_ADDRESS   0x0012002000ULL
#define PU_UART1_BASE_ADDRESS   0x0012007000ULL

#define UART0 ((volatile Uart_t*)SPIO_UART0_BASE_ADDRESS)
#define UART1 ((volatile Uart_t*)SPIO_UART1_BASE_ADDRESS)
#define PU_UART0 ((volatile Uart_t*)PU_UART0_BASE_ADDRESS)
#define PU_UART1 ((volatile Uart_t*)PU_UART1_BASE_ADDRESS)

#endif
