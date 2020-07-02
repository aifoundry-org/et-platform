#ifndef SERIAL_H
#define SERIAL_H

#include "serial_device.h"

#include <stdint.h>

#define DEBUG_WRITE(x) SERIAL_write(UART0, x, sizeof(x) - 1)

int SERIAL_init(uintptr_t uartRegs);
void SERIAL_write(uintptr_t uartRegs, const char* const string, int length);

#endif // SERIAL_H
