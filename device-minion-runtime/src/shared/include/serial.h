#ifndef SERIAL_H
#define SERIAL_H

#include "serial_device.h"
#include "UART.h"

#include <stdint.h>

#define DEBUG_WRITE(x) SERIAL_write(UART0, x, sizeof(x) - 1)

int SERIAL_init(volatile SPIO_UART_t* const uartRegs);
void SERIAL_write(volatile SPIO_UART_t* const uartRegs, const char* const string, int length);

#endif // SERIAL_H
