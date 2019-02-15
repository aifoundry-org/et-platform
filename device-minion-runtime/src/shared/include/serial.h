#ifndef SERIAL_H
#define SERIAL_H

#include "UART.h"

#include <stdint.h>

int SERIAL_init(SPIO_UART_t* const uartRegs);
void SERIAL_write(SPIO_UART_t* const uartRegs, const char* const string, unsigned int length);

#endif // SERIAL_H
